// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

/** @brief Filename the generated .c's `#include` and the copy in each benchmarkDir share. */
inline constexpr const char *kArgvCHarnessHeaderName = "argv_c_harness.h";

/** @brief Verbatim, null-terminated contents of src/common/argv_c_harness.h, embedded at build time. */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc23-extensions" // #embed is C23; clang accepts it in C++
inline constexpr char kArgvCHarnessHeaderContents[] = {
#embed "../argv_c_harness.h"
    , 0};
#pragma clang diagnostic pop
