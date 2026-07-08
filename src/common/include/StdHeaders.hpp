// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

/**
 * @file StdHeaders.hpp
 * @brief Registry mapping standard/POSIX type names to their defining headers.
 *
 * Used by the transform step to re-inject the standard headers that include
 * stripping leaves missing. Each entry also carries a {@code HeaderCategory} so
 * future passes can filter benchmarks by logical structure (concurrency, file
 * I/O, etc.).
 */

/**
 * @brief Logical grouping for a standard header, for future structure-based filtering.
 */
enum class HeaderCategory {
  Core,
  IO,
  String,
  Math,
  Concurrency,
  Signal,
  Time,
};

/**
 * @brief A standard type's defining header together with its logical category.
 */
struct StdHeaderInfo {
  std::string header;       ///< Header that defines the type (e.g. "stdint.h").
  HeaderCategory category;  ///< Logical grouping of the header.
};

/**
 * @brief Maps a standard/POSIX type name to the header that defines it.
 */
inline const std::unordered_map<std::string, StdHeaderInfo> kStdTypeHeaders = {
    // <stdbool.h>
    {"bool", {"stdbool.h", HeaderCategory::Core}},

    // <stddef.h>
    {"size_t", {"stddef.h", HeaderCategory::Core}},
    {"ptrdiff_t", {"stddef.h", HeaderCategory::Core}},
    {"wchar_t", {"stddef.h", HeaderCategory::Core}},
    {"max_align_t", {"stddef.h", HeaderCategory::Core}},

    // <stdint.h>
    {"int8_t", {"stdint.h", HeaderCategory::Core}},
    {"int16_t", {"stdint.h", HeaderCategory::Core}},
    {"int32_t", {"stdint.h", HeaderCategory::Core}},
    {"int64_t", {"stdint.h", HeaderCategory::Core}},
    {"uint8_t", {"stdint.h", HeaderCategory::Core}},
    {"uint16_t", {"stdint.h", HeaderCategory::Core}},
    {"uint32_t", {"stdint.h", HeaderCategory::Core}},
    {"uint64_t", {"stdint.h", HeaderCategory::Core}},
    {"intptr_t", {"stdint.h", HeaderCategory::Core}},
    {"uintptr_t", {"stdint.h", HeaderCategory::Core}},
    {"intmax_t", {"stdint.h", HeaderCategory::Core}},
    {"uintmax_t", {"stdint.h", HeaderCategory::Core}},
    {"int_least8_t", {"stdint.h", HeaderCategory::Core}},
    {"int_least16_t", {"stdint.h", HeaderCategory::Core}},
    {"int_least32_t", {"stdint.h", HeaderCategory::Core}},
    {"int_least64_t", {"stdint.h", HeaderCategory::Core}},
    {"uint_least8_t", {"stdint.h", HeaderCategory::Core}},
    {"uint_least16_t", {"stdint.h", HeaderCategory::Core}},
    {"uint_least32_t", {"stdint.h", HeaderCategory::Core}},
    {"uint_least64_t", {"stdint.h", HeaderCategory::Core}},
    {"int_fast8_t", {"stdint.h", HeaderCategory::Core}},
    {"int_fast16_t", {"stdint.h", HeaderCategory::Core}},
    {"int_fast32_t", {"stdint.h", HeaderCategory::Core}},
    {"int_fast64_t", {"stdint.h", HeaderCategory::Core}},
    {"uint_fast8_t", {"stdint.h", HeaderCategory::Core}},
    {"uint_fast16_t", {"stdint.h", HeaderCategory::Core}},
    {"uint_fast32_t", {"stdint.h", HeaderCategory::Core}},
    {"uint_fast64_t", {"stdint.h", HeaderCategory::Core}},

    // <stdio.h>
    {"FILE", {"stdio.h", HeaderCategory::IO}},
    {"fpos_t", {"stdio.h", HeaderCategory::IO}},

    // <stdlib.h>
    {"div_t", {"stdlib.h", HeaderCategory::Core}},
    {"ldiv_t", {"stdlib.h", HeaderCategory::Core}},
    {"lldiv_t", {"stdlib.h", HeaderCategory::Core}},

    // <string.h> — no types beyond size_t

    // <time.h>
    {"time_t", {"time.h", HeaderCategory::Time}},
    {"clock_t", {"time.h", HeaderCategory::Time}},
    {"tm", {"time.h", HeaderCategory::Time}},
    {"timespec", {"time.h", HeaderCategory::Time}},

    // <signal.h>
    {"sig_atomic_t", {"signal.h", HeaderCategory::Signal}},

    // <math.h>
    {"float_t", {"math.h", HeaderCategory::Math}},
    {"double_t", {"math.h", HeaderCategory::Math}},

    // <threads.h> / <pthread.h>
    {"thrd_t", {"threads.h", HeaderCategory::Concurrency}},
    {"mtx_t", {"threads.h", HeaderCategory::Concurrency}},
    {"cnd_t", {"threads.h", HeaderCategory::Concurrency}},
    {"tss_t", {"threads.h", HeaderCategory::Concurrency}},
    {"pthread_t", {"pthread.h", HeaderCategory::Concurrency}},
    {"pthread_mutex_t", {"pthread.h", HeaderCategory::Concurrency}},
    {"pthread_cond_t", {"pthread.h", HeaderCategory::Concurrency}},
    {"pthread_attr_t", {"pthread.h", HeaderCategory::Concurrency}},
    {"pthread_rwlock_t", {"pthread.h", HeaderCategory::Concurrency}},
    {"pthread_key_t", {"pthread.h", HeaderCategory::Concurrency}},
    {"pthread_once_t", {"pthread.h", HeaderCategory::Concurrency}},

    // <stdarg.h>
    {"va_list", {"stdarg.h", HeaderCategory::Core}},

    // <errno.h> — no types, just macros

    // <inttypes.h>
    {"imaxdiv_t", {"inttypes.h", HeaderCategory::Core}},

    // <wchar.h>
    {"wint_t", {"wchar.h", HeaderCategory::String}},
    {"mbstate_t", {"wchar.h", HeaderCategory::String}},

    // <locale.h>
    {"lconv", {"locale.h", HeaderCategory::Core}},

    // POSIX common types
    {"ssize_t", {"unistd.h", HeaderCategory::IO}},
    {"pid_t", {"sys/types.h", HeaderCategory::Core}},
    {"uid_t", {"sys/types.h", HeaderCategory::Core}},
    {"gid_t", {"sys/types.h", HeaderCategory::Core}},
    {"off_t", {"sys/types.h", HeaderCategory::Core}},
    {"mode_t", {"sys/types.h", HeaderCategory::Core}},
    {"dev_t", {"sys/types.h", HeaderCategory::Core}},
    {"ino_t", {"sys/types.h", HeaderCategory::Core}},
    {"nlink_t", {"sys/types.h", HeaderCategory::Core}},

    // <sys/socket.h>
    {"socklen_t", {"sys/socket.h", HeaderCategory::IO}},
    {"sa_family_t", {"sys/socket.h", HeaderCategory::IO}},
    {"sockaddr", {"sys/socket.h", HeaderCategory::IO}},

    // <netinet/in.h>
    {"in_addr_t", {"netinet/in.h", HeaderCategory::IO}},
    {"in_port_t", {"netinet/in.h", HeaderCategory::IO}},

    // <dirent.h>
    {"DIR", {"dirent.h", HeaderCategory::IO}},

    // <sys/stat.h>
    {"stat", {"sys/stat.h", HeaderCategory::IO}},
};

/**
 * @brief Looks up the header and category for a standard type name.
 *
 * @param typeName Type name as spelled in source (e.g. "size_t").
 * @return The header/category info, or std::nullopt if the type is not standard.
 */
inline std::optional<StdHeaderInfo> stdHeaderForType(const std::string &typeName) {
  auto it = kStdTypeHeaders.find(typeName);
  if (it == kStdTypeHeaders.end())
    return std::nullopt;
  return it->second;
}

/**
 * @brief Returns the set of headers belonging to a logical category.
 *
 * @param cat Category to collect headers for.
 * @return All distinct header names whose entries are tagged with {@code cat}.
 */
inline std::unordered_set<std::string> headersForCategory(HeaderCategory cat) {
  std::unordered_set<std::string> result;
  for (const auto &[type, info] : kStdTypeHeaders) {
    if (info.category == cat)
      result.insert(info.header);
  }
  return result;
}
