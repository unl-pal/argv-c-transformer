// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <regex>
#include <string>
#include <utility>

/**
 * @brief Three-state gate for a feature flag: whether the config requires a
 * function to have the feature present, forbids it, or doesn't care.
 */
enum class FeatureGate { Ignore, Require, Forbid };

/**
 * @brief Everything the INI-style properties file can configure, parsed once
 * and shared by all three stages (filter, transform, verify).
 *
 * The complexity thresholds and feature gates are applied per function by the
 * filter stage and re-applied by the verify stage after transformation. Path
 * fields are left empty when the config doesn't set them; each stage driver
 * applies its own defaults (and CLI overrides) on top.
 */
struct PipelineConfig {
  /// Per-function complexity thresholds: metric name → [min, max]. Defaults
  /// to {0, 9999}, meaning no filtering unless explicitly configured.
  std::map<std::string, std::pair<int, int>> complexity = {
      {"CallFunc", {0, 9999}}, {"ForLoops", {0, 9999}}, {"IfStmt", {0, 9999}},
      {"Param", {0, 9999}},    {"WhileLoops", {0, 9999}}, {"Operations", {0,9999}},
  };

  /// Per-function feature gates: feature name → ignore|require|forbid.
  std::map<std::string, FeatureGate> features = {
      {"Concurrency", FeatureGate::Ignore},
      {"FloatingPoint", FeatureGate::Ignore},
  };

  /// File-level integer settings (booleans are stored as 0/1).
  std::map<std::string, int> fileSettings = {
      {"debugLevel", 0},       {"minFileLoC", 0},        {"maxFileLoC", 9999},
      {"fileTimeoutSecs", 60}, {"keepCompilesOnly", 1},
  };

  std::string databaseDir;  ///< Input tree for the filter stage ("" = unset).
  std::string filterDir;    ///< Filter output / transform input ("" = unset).
  std::string transformDir; ///< Transform output / verify input ("" = unset).
  std::string benchmarkDir; ///< Verify output: the final benchmarks ("" = unset).
};

/**
 * @brief Strips leading and trailing spaces/tabs.
 */
inline std::string trim(const std::string &s) {
  size_t start = s.find_first_not_of(" \t");
  size_t end = s.find_last_not_of(" \t");
  if (start == std::string::npos)
    return "";
  return s.substr(start, end - start + 1);
}

/**
 * @brief Parses an INI-style config file and returns raw key/value string pairs.
 *
 * Lines that do not match the {@code key = value} pattern (comments, blank
 * lines, section headers) are silently skipped. Each tool is responsible for
 * interpreting its own keys from the returned map; unknown keys are not
 * reported here.
 *
 * @param configFile Path to the INI-style properties file.
 * @return Map of key to raw string value for every matched line, or an empty
 *         map if the file does not exist or cannot be opened.
 */
inline std::map<std::string, std::string> parseIniFile(const std::string &configFile) {
  std::map<std::string, std::string> result;
  if (!std::filesystem::exists(configFile))
    return result;
  std::ifstream file(configFile);
  if (!file.is_open())
    return result;
  std::regex pattern(R"(^\s*(\w+)\s*=\s*([0-9]+|[\w\s,\-]+|[\w/\-_.]+)$)");
  std::string line;
  std::smatch match;
  while (std::getline(file, line)) {
    if (std::regex_search(line, match, pattern))
      result[match[1]] = match[2];
  }
  return result;
}

/**
 * @brief Parses a complexity threshold value into a {min, max} pair.
 *
 * Accepts a "min,max" range (e.g. "1,10"), a bare value treated as a minimum
 * with no upper bound (e.g. "2" == "2,9999"), or a "," followed by just a max
 * (e.g. ",10" == "0,10").
 *
 * @param value Raw config value, already matched as key=value.
 * @return The parsed pair, or std::nullopt if the value is malformed.
 */
inline std::optional<std::pair<int, int>> parseComplexityValue(const std::string &value) {
  std::string trimmed = trim(value);
  size_t comma = trimmed.find(',');
  std::string minStr = trim(trimmed.substr(0, comma));
  std::string maxStr = comma == std::string::npos ? "9999" : trim(trimmed.substr(comma + 1));

  // Note: this will allow and parse "5abc" as 5
  try {
    int min = minStr.empty() ? 0 : std::stoi(minStr);
    int max = maxStr.empty() ? 9999 : std::stoi(maxStr);
    return std::make_pair(min, max);
  } catch (...) {
    return std::nullopt;
  }
}

/**
 * @brief Parses the INI-style properties file into a PipelineConfig.
 *
 * All keys any stage understands are interpreted here, so a typo'd key warns
 * exactly once regardless of which binary runs. Downloader.py-only keys are
 * recognised and skipped. A missing file warns and returns the defaults.
 *
 * Path existence checks and directory creation are left to the stage
 * drivers, which know which paths they read vs. write.
 *
 * @param configFile Path to the INI-style properties file ("" = defaults only).
 * @return The parsed configuration.
 */
inline PipelineConfig parsePipelineConfig(const std::string &configFile) {
  PipelineConfig config;
  if (configFile.empty())
    return config;
  if (!std::filesystem::exists(configFile)) {
    std::cerr << "Config file not found: " << configFile << " — using defaults" << std::endl;
    return config;
  }

  for (const auto &[key, value] : parseIniFile(configFile)) {
    if (config.complexity.count(key)) {
      std::optional<std::pair<int, int>> range = parseComplexityValue(value);
      if (range && range->first > range->second) {
        std::cerr << "Warning: complexity key '" << key << "' has min (" << range->first
                  << ") greater than max (" << range->second << ") — ignoring value '" << value
                  << "'" << std::endl;
      } else if (range) {
        config.complexity.at(key) = *range;
      } else {
        std::cerr << "Warning: complexity key '" << key
                  << "' expects 'min,max', 'min', or ',max' — ignoring value '" << value << "'"
                  << std::endl;
      }
    } else if (config.features.count(key)) {
      std::string v = trim(value);
      std::transform(v.begin(), v.end(), v.begin(), ::tolower);
      if (v == "require") {
        config.features.at(key) = FeatureGate::Require;
      } else if (v == "forbid") {
        config.features.at(key) = FeatureGate::Forbid;
      } else if (v == "ignore" || v.empty()) {
        config.features.at(key) = FeatureGate::Ignore;
      } else {
        std::cerr << "Warning: unrecognised gate '" << value << "' for feature '" << key
                  << "' — ignoring" << std::endl;
      }
    } else if (config.fileSettings.count(key)) {
      try {
        config.fileSettings.at(key) = std::stoi(value);
      } catch (...) {
      }
      if (value == "false" || value == "False") {
        config.fileSettings.at(key) = 0;
      } else if (value == "true" || value == "True") {
        config.fileSettings.at(key) = 1;
      }
    } else if (key == "databaseDir") {
      config.databaseDir = value;
    } else if (key == "filterDir") {
      config.filterDir = value;
    } else if (key == "transformDir") {
      config.transformDir = value;
    } else if (key == "benchmarkDir") {
      config.benchmarkDir = value;
    } else if (key == "csv" || key == "downloadDir" || key == "language" ||
               key == "projectCount" || key == "minNumStars" || key == "minRepoLoc" ||
               key == "stars" || key == "size") {
      // consumed by Downloader.py; not a pipeline key
    } else if (key == "wipeOldBenchmarks" || key == "useNonStdHeaders") {
      std::cerr << "Config key '" << key << "' has been removed — ignoring" << std::endl;
    } else if (key == "debug") {
      // silently ignored: replaced by debugLevel
    } else {
      std::cerr << "Unknown config key: " << key << std::endl;
    }
  }
  return config;
}
