// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <algorithm>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <functional>
#include <string>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

/**
 * @brief Resolves the {@code nproc} config setting to an actual worker
 * count: the configured value if positive, else the detected hardware
 * concurrency (floored at 1 if detection fails).
 */
inline int resolveWorkerCount(int configuredNproc) {
  if (configuredNproc > 0)
    return configuredNproc;
  unsigned hw = std::thread::hardware_concurrency();
  return static_cast<int>(std::max(1u, hw));
}

/**
 * @brief The per-file work a worker pool isolates, shared by every caller of
 * {@code runWorkerPool}.
 *
 * Mirrors what each stage's old FooFileIsolated hand-rolled: a child body
 * that does the real work and exits with its result, an in-process fallback
 * for when fork() itself fails, and cleanup for a child that got killed.
 */
struct IsolatedWork {
  /**
   * Runs in the forked child. Must not return: do the work, flush stdout
   * and stderr (`_exit` skips C++ stream flushing), then `_exit(1)` on
   * success or `_exit(0)` otherwise.
   */
  std::function<void(const std::filesystem::path &)> child;

  /** Fallback run synchronously in the parent when fork() itself fails. */
  std::function<bool(const std::filesystem::path &)> runInProcess;

  /** Removes any output a killed/crashed/timed-out child left behind. */
  std::function<void(const std::filesystem::path &)> cleanupPartial;

  /** (debugLevel, message) - routed to each stage's own debugLog. */
  std::function<void(int, const std::string &)> debugLog;
};

struct WorkerPoolJob {
  pid_t pid;
  std::filesystem::path file;
  time_t deadline;
};

/**
 * @brief Runs {@code work} over every file in {@code files}, isolating each
 * in its own forked child the way the old per-stage FooFileIsolated did, but
 * running up to {@code workers} children concurrently instead of one at a
 * time.
 *
 * Each file gets its own wall-clock budget of {@code timeoutSecs} starting
 * from when its child is forked, independent of how long its pool-mates
 * take.
 *
 * @return count of files whose child exited reporting success (status 1).
 */
inline int runWorkerPool(const std::vector<std::filesystem::path> &files, int workers,
                          int timeoutSecs, const IsolatedWork &work) {
  std::vector<WorkerPoolJob> inFlight;
  size_t next = 0;
  int produced = 0;

  auto killAndReap = [&](WorkerPoolJob &job, const char *reason) {
    int status = 0;
    kill(job.pid, SIGKILL);
    waitpid(job.pid, &status, 0);
    work.debugLog(0, std::string(reason) + ": " + job.file.string());
    work.cleanupPartial(job.file);
  };

  while (next < files.size() || !inFlight.empty()) {
    while (static_cast<int>(inFlight.size()) < workers && next < files.size()) {
      const std::filesystem::path &file = files[next++];
      pid_t pid = fork();
      if (pid < 0) {
        work.debugLog(0, "fork failed, running in-process: " + file.string());
        produced += work.runInProcess(file) ? 1 : 0;
        continue;
      }
      if (pid == 0) {
        work.child(file);
        _exit(0); // safety net; work.child is expected to _exit itself
      }
      inFlight.push_back({pid, file, time(nullptr) + timeoutSecs});
    }

    bool reaped = false;
    for (auto it = inFlight.begin(); it != inFlight.end();) {
      int status = 0;
      pid_t done = waitpid(it->pid, &status, WNOHANG);
      if (done == it->pid) {
        if (WIFEXITED(status)) {
          produced += WEXITSTATUS(status) == 1 ? 1 : 0;
        } else {
          work.debugLog(0, "crashed (signal " + std::to_string(WTERMSIG(status)) +
                                "), skipping: " + it->file.string());
          work.cleanupPartial(it->file);
        }
        it = inFlight.erase(it);
        reaped = true;
      } else if (done < 0 && errno != EINTR) {
        killAndReap(*it, "waitpid failed, killing");
        it = inFlight.erase(it);
        reaped = true;
      } else if (time(nullptr) >= it->deadline) {
        killAndReap(*it, "Timeout, killing");
        it = inFlight.erase(it);
        reaped = true;
      } else {
        ++it;
      }
    }

    // Nothing to reap and no free slot (or nothing left to submit): avoid a
    // busy-spin while children run.
    if (!reaped && !inFlight.empty()) {
      struct timespec nap = {0, 20 * 1000 * 1000}; // 20ms
      nanosleep(&nap, nullptr);
    }
  }
  return produced;
}
