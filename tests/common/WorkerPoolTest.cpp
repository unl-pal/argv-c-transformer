// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "WorkerPool.hpp"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <gtest/gtest.h>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Unit tests for the shared worker pool
// ---------------------------------------------------------------------------
//
// The stages differ only in what their IsolatedWork callbacks do, so these
// drive the pool with synthetic work that exits however a test needs.
// cleanupPartial runs in the parent, so a test can record its calls directly.

namespace {

/** Collects the pool's parent-side callbacks for assertions. */
struct Recorder {
  std::vector<std::string> cleaned;

  IsolatedWork work(std::function<void(const fs::path &)> child,
                    std::function<bool(const fs::path &)> inProcess = nullptr) {
    IsolatedWork w;
    w.child = std::move(child);
    w.runInProcess = inProcess ? std::move(inProcess) : [](const fs::path &) { return false; };
    w.cleanupPartial = [this](const fs::path &p) { cleaned.push_back(p.filename().string()); };
    w.debugLog = [](int, const std::string &) {};
    w.label = "test";
    return w;
  }
};

std::vector<fs::path> paths(std::initializer_list<std::string> names) {
  std::vector<fs::path> out;
  for (const std::string &n : names)
    out.emplace_back(n);
  return out;
}

/** A child that exits with `code` for every file. */
std::function<void(const fs::path &)> childExiting(int code) {
  return [code](const fs::path &) { _exit(code); };
}

} // namespace

TEST(ResolveWorkerCount, PositiveConfigValueIsUsedVerbatim) {
  EXPECT_EQ(resolveWorkerCount(1), 1);
  unsigned hw = std::thread::hardware_concurrency();
  if (hw > 1)
    EXPECT_EQ(resolveWorkerCount(static_cast<int>(hw) - 1), static_cast<int>(hw) - 1);
}

TEST(ResolveWorkerCount, OversubscriptionIsClampedToTheCoreCountWithAWarning) {
  unsigned hw = std::thread::hardware_concurrency();
  if (hw == 0)
    GTEST_SKIP() << "no detectable core count";

  testing::internal::CaptureStderr();
  int workers = resolveWorkerCount(static_cast<int>(hw) + 4);
  std::string warning = testing::internal::GetCapturedStderr();

  EXPECT_EQ(workers, static_cast<int>(hw));
  EXPECT_NE(warning.find("exceeds"), std::string::npos) << warning;
}

// At or below the core count the configured value is used as-is, silently.
TEST(ResolveWorkerCount, CountWithinTheCoreCountIsNotWarnedAbout) {
  unsigned hw = std::thread::hardware_concurrency();
  if (hw == 0)
    GTEST_SKIP() << "no detectable core count";

  testing::internal::CaptureStderr();
  int workers = resolveWorkerCount(static_cast<int>(hw));
  std::string warning = testing::internal::GetCapturedStderr();

  EXPECT_EQ(workers, static_cast<int>(hw));
  EXPECT_TRUE(warning.empty()) << warning;
}

TEST(ResolveWorkerCount, ZeroAutoSizesBelowCoreCountButAtLeastOne) {
  int workers = resolveWorkerCount(0);
  EXPECT_GE(workers, 1);
  unsigned hw = std::thread::hardware_concurrency();
  if (hw > 0)
    EXPECT_LE(workers, static_cast<int>(hw));
}

TEST(WorkerPool, ProducedFilesAreCountedAndLeftAlone) {
  Recorder rec;
  IsolatedWork work = rec.work(childExiting(kProducedExit));

  WorkerPoolResult result = runWorkerPool(paths({"a.c", "b.c"}), 2, 30, work);

  EXPECT_EQ(result.produced, 2);
  EXPECT_EQ(result.declined, 0);
  EXPECT_EQ(result.failed, 0);
  EXPECT_TRUE(rec.cleaned.empty());
}

// A child that exits under its own control owns its residue - including the
// non-compiling .c verify keeps under keepCompilesOnly=false - so the pool
// must leave the filesystem alone.
TEST(WorkerPool, DeclineDoesNotTriggerCleanup) {
  Recorder rec;
  IsolatedWork work = rec.work(childExiting(kDeclinedExit));

  WorkerPoolResult result = runWorkerPool(paths({"a.c"}), 2, 30, work);

  EXPECT_EQ(result.declined, 1);
  EXPECT_EQ(result.produced, 0);
  EXPECT_TRUE(rec.cleaned.empty());
}

TEST(WorkerPool, CrashedChildIsFailedAndCleanedUp) {
  Recorder rec;
  IsolatedWork work = rec.work([](const fs::path &) { abort(); });

  WorkerPoolResult result = runWorkerPool(paths({"a.c"}), 2, 30, work);

  EXPECT_EQ(result.failed, 1);
  EXPECT_EQ(result.produced, 0);
  EXPECT_EQ(rec.cleaned, std::vector<std::string>{"a.c"});
}

// Any exit code the contract does not name is a failure, not a decline - the
// point of keeping kDeclinedExit off 1.
TEST(WorkerPool, UnexpectedExitCodeIsFailure) {
  Recorder rec;
  IsolatedWork work = rec.work([](const fs::path &) { _exit(1); });

  WorkerPoolResult result = runWorkerPool(paths({"a.c"}), 2, 30, work);

  EXPECT_EQ(result.failed, 1);
  EXPECT_EQ(result.declined, 0);
  EXPECT_EQ(rec.cleaned, std::vector<std::string>{"a.c"});
}

TEST(WorkerPool, ChildOverrunningTheBudgetIsKilledAndCleanedUp) {
  Recorder rec;
  IsolatedWork work = rec.work([](const fs::path &) {
    sleep(30);
    _exit(kProducedExit);
  });

  WorkerPoolResult result = runWorkerPool(paths({"a.c"}), 2, 1, work);

  EXPECT_EQ(result.failed, 1);
  EXPECT_EQ(result.produced, 0);
  EXPECT_EQ(rec.cleaned, std::vector<std::string>{"a.c"});
}

// One slow file must not stop the pool from finishing the rest.
TEST(WorkerPool, TimeoutIsPerFileNotPerPool) {
  Recorder rec;
  IsolatedWork work = rec.work([](const fs::path &p) {
    if (p.filename() == "slow.c")
      sleep(30);
    _exit(kProducedExit);
  });

  WorkerPoolResult result = runWorkerPool(paths({"slow.c", "a.c", "b.c"}), 3, 1, work);

  EXPECT_EQ(result.produced, 2);
  EXPECT_EQ(result.failed, 1);
  EXPECT_EQ(rec.cleaned, std::vector<std::string>{"slow.c"});
}

// More files than slots: every file is still dispatched exactly once, and the
// counts account for all of them.
TEST(WorkerPool, EveryFileIsProcessedWhenFilesOutnumberWorkers) {
  Recorder rec;
  IsolatedWork work = rec.work([](const fs::path &p) {
    // Alternate outcomes so the reap loop is exercised on both branches.
    _exit(p.filename().string()[0] % 2 ? kProducedExit : kDeclinedExit);
  });

  std::vector<fs::path> files;
  for (int i = 0; i < 20; i++)
    files.emplace_back(std::string(1, static_cast<char>('a' + i)) + ".c");

  WorkerPoolResult result = runWorkerPool(files, 3, 30, work);

  EXPECT_EQ(result.produced + result.declined + result.failed, static_cast<int>(files.size()));
  EXPECT_EQ(result.failed, 0);
  EXPECT_TRUE(rec.cleaned.empty());
}

TEST(WorkerPool, EmptyInputProducesEmptyResult) {
  Recorder rec;
  IsolatedWork work = rec.work(childExiting(kProducedExit));

  WorkerPoolResult result = runWorkerPool({}, 4, 30, work);

  EXPECT_EQ(result.produced, 0);
  EXPECT_EQ(result.declined, 0);
  EXPECT_EQ(result.failed, 0);
}

// A child's stderr goes to a private log the parent flushes on reap, so the
// pool must not leave those temp files behind.
TEST(WorkerPool, ChildLogDirectoryIsRemovedAfterTheRun) {
  Recorder rec;
  IsolatedWork work = rec.work([](const fs::path &) {
    fprintf(stderr, "noise from the child\n");
    _exit(kProducedExit);
  });

  runWorkerPool(paths({"a.c"}), 2, 30, work);

  fs::path logDir = fs::temp_directory_path() / ("argv-c-pool-" + std::to_string(getpid()));
  EXPECT_FALSE(fs::exists(logDir));
}

