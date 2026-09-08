// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <algorithm>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

/**
 * @brief Resolves the {@code nproc} config setting to an actual worker count:
 * the configured value if positive, capped at the detected core count, else
 * three quarters of the detected cores (floored at 1).
 */
inline int resolveWorkerCount(int configuredNproc) {
  unsigned hw = std::thread::hardware_concurrency();
  if (configuredNproc > 0) {
    if (hw > 0 && configuredNproc > static_cast<int>(hw)) {
      std::cerr << "Warning: 'nproc' (" << configuredNproc << ") exceeds the " << hw
                << " detected cores - using " << hw << std::endl;
      return static_cast<int>(hw);
    }
    return configuredNproc;
  }
  if (hw == 0)
    return 1;
  return static_cast<int>(std::max(1u, hw * 3 / 4));
}

/** Child exit code for "produced an output file". */
inline constexpr int kProducedExit = 0;

/**
 * Child exit code for "ran fine, produced nothing" - a declined file, not an
 * error. Deliberately not 1.
 */
inline constexpr int kDeclinedExit = 2;

/** Per-outcome file **counts** (not return codes) for one {@code runWorkerPool} call. */
struct WorkerPoolResult {
  int produced = 0; ///< Children that exited {@code kProducedExit}.
  int declined = 0; ///< Children that exited {@code kDeclinedExit}.
  int failed = 0;   ///< Crashed, timed out, or exited any other code.
};

/**
 * @brief The per-file work a worker pool isolates, shared by every caller of
 * {@code runWorkerPool}.
 */
struct IsolatedWork {
  /**
   * Runs in the forked child. Perform task, cleanup files if neccessary, flush
   * stdout and stderr (`_exit` skips C++ stream flushing), then `_exit(kProducedExit)` or
   * `_exit(kDeclinedExit)`.
   */
  std::function<void(const std::filesystem::path &)> child;

  /**
   * Fallback run synchronously in the parent when fork() itself fails.
   * Returning false counts as a decline, not a failure.
   */
  std::function<bool(const std::filesystem::path &)> runInProcess;

  /**
   * Removes the output of a child that died before it could tidy up after
   * itself - killed, crashed, or timed out. Never called for a child that
   * exited under its own control, which owns its own residue.
   */
  std::function<void(const std::filesystem::path &)> cleanupPartial;

  /** (debugLevel, message) - routed to each stage's own debugLog. */
  std::function<void(int, const std::string &)> debugLog;

  /** Stage name used to tag the pool's progress line, e.g. "filter". */
  std::string label;
};

/** One forked child the pool is currently waiting on. */
struct WorkerPoolJob {
  pid_t pid;
  std::filesystem::path file;
  time_t deadline;               ///< Absolute wall-clock time after which the child is killed.
  std::filesystem::path logPath; ///< Private file this child's fd 2 is redirected to.
};

/**
 * @brief Prints a reaped child's buffered stderr as one block tagged with its
 * file, then deletes the log. A no-op if the child wrote nothing.
 *
 * Only the parent ever prints, so concurrent children's logs never interleave.
 */
inline void flushChildLog(const WorkerPoolJob &job) {
  std::ifstream in(job.logPath);
  if (in) {
    std::ostringstream buf;
    buf << in.rdbuf();
    std::string content = buf.str();
    if (!content.empty()) {
      std::cerr << "----- " << job.file.string() << " -----\n" << content;
      if (content.back() != '\n')
        std::cerr << '\n';
    }
  }
  std::error_code ec;
  std::filesystem::remove(job.logPath, ec);
}

/**
 * @brief Runs {@code work} over every file in {@code files}, isolating each
 * in its own forked child running up to {@code workers} children concurrently.
 *
 * Each file gets its own wall-clock budget of {@code timeoutSecs} starting
 * from when its child is forked.
 *
 * @return per-outcome counts; see {@code WorkerPoolResult}.
 */
inline WorkerPoolResult runWorkerPool(const std::vector<std::filesystem::path> &files, int workers,
                                      int timeoutSecs, const IsolatedWork &work) {
  std::error_code ec;
  std::filesystem::path logDir =
      std::filesystem::temp_directory_path(ec) / ("argv-c-pool-" + std::to_string(getpid()));
  if (ec)
    logDir = std::filesystem::path(".") / ("argv-c-pool-" + std::to_string(getpid()));
  // allow fallback to stderr interleaved if logDir fails
  std::filesystem::create_directories(logDir, ec);

  std::vector<WorkerPoolJob> inFlight;
  size_t next = 0;
  WorkerPoolResult result;

  // Above level 0 the carriage-returned line is interleaved with debugLog's
  // stderr output rather than updating in place. The flush also keeps stdout
  // empty at every fork below - an inherited dirty buffer gets flushed again
  // by every child.
  auto reportProgress = [&]() {
    if (!files.empty())
      std::cout << "\r[" << work.label << "] " << (result.produced + result.declined + result.failed)
                << "/" << files.size() << " processed" << std::flush;
  };
  reportProgress();

  auto killAndReap = [&](WorkerPoolJob &job, const char *reason) {
    int status = 0;
    kill(job.pid, SIGKILL);
    waitpid(job.pid, &status, 0);
    flushChildLog(job);
    work.debugLog(0, std::string(reason) + ": " + job.file.string());
    work.cleanupPartial(job.file);
  };

  while (next < files.size() || !inFlight.empty()) {
    while (static_cast<int>(inFlight.size()) < workers && next < files.size()) {
      size_t idx = next;
      const std::filesystem::path &file = files[next++];
      std::filesystem::path logPath = logDir / (std::to_string(idx) + ".log");
      pid_t pid = fork();
      if (pid < 0) {
        work.debugLog(0, "fork failed, running in-process: " + file.string());
        if (work.runInProcess(file))
          result.produced++;
        else
          result.declined++;
        reportProgress();
        continue;
      }
      if (pid == 0) {
        int fd = open(logPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0) {
          dup2(fd, STDERR_FILENO);
          close(fd);
        }
        work.child(file);
        _exit(1); // safety net; work.child is expected to _exit itself
      }
      inFlight.push_back({pid, file, time(nullptr) + timeoutSecs, logPath});
    }

    bool reaped = false;
    for (auto it = inFlight.begin(); it != inFlight.end();) {
      int status = 0;
      pid_t done = waitpid(it->pid, &status, WNOHANG);
      if (done == it->pid) {
        flushChildLog(*it);
        if (WIFEXITED(status) && WEXITSTATUS(status) == kProducedExit) {
          result.produced++;
        } else if (WIFEXITED(status) && WEXITSTATUS(status) == kDeclinedExit) {
          // No cleanup: the child exited under its own control
          result.declined++;
          work.debugLog(1, "no output: " + it->file.string());
        } else if (WIFEXITED(status)) {
          result.failed++;
          work.debugLog(0, "failed (exit " + std::to_string(WEXITSTATUS(status)) +
                                "), skipping: " + it->file.string());
          work.cleanupPartial(it->file);
        } else {
          result.failed++;
          work.debugLog(0, "crashed (signal " + std::to_string(WTERMSIG(status)) +
                                "), skipping: " + it->file.string());
          work.cleanupPartial(it->file);
        }
        it = inFlight.erase(it);
        reaped = true;
      } else if (done < 0 && errno != EINTR) {
        killAndReap(*it, "waitpid failed, killing");
        result.failed++;
        it = inFlight.erase(it);
        reaped = true;
      } else if (time(nullptr) >= it->deadline) {
        killAndReap(*it, "Timeout, killing");
        result.failed++;
        it = inFlight.erase(it);
        reaped = true;
      } else {
        ++it;
      }
    }

    if (reaped)
      reportProgress();

    // Nothing to reap and no free slot (or nothing left to submit): avoid a
    // busy-spin while children run.
    if (!reaped && !inFlight.empty()) {
      struct timespec nap = {0, 20 * 1000 * 1000}; // 20ms
      nanosleep(&nap, nullptr);
    }
  }

  if (!files.empty())
    std::cout << std::endl;

  std::filesystem::remove_all(logDir, ec);
  return result;
}
