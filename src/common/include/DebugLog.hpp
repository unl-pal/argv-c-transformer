#pragma once

#include <iostream>
#include <string>

/**
 * @file DebugLog.hpp
 * @brief Process-global verbosity for log gating.
 *
 * Set once from the config's {@code debugLevel}. Consumers buried deep in the
 * Clang chain (and the forked transform children, which inherit the value)
 * read it without threading {@code debugLevel} through every layer.
 *
 * Levels: 0 silent (summary + real errors only), 1 per-file progress,
 * 2 per-function decisions, 3 everything.
 */
/**
 * @brief Accessor for the process-global verbosity level.
 *
 * @return A reference to the level, so callers can read or set it.
 */
inline int &globalDebugLevel() {
  static int level = 0;
  return level;
}

/**
 * @brief Writes a message to stderr, gated on the global verbosity level.
 *
 * @param level Minimum verbosity at which {@code msg} should be printed.
 * @param msg   The message to write (a newline is appended).
 */
inline void debugLog(int level, const std::string &msg) {
  if (globalDebugLevel() >= level)
    std::cerr << msg << std::endl;
}
