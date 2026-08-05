// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <optional>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>

/** IncludeIndex is a fallback utility for finding header files when they aren't
 * within the source's directory tree. Without a compile_commands.json we must
 * find the correct headers and use a nearness heuristic for disambuguation.
 */

/**
 * @brief Maps header basenames to every directory under a root tree that
 * contains a file with that name.
 */
class HeaderIndex {
public:
  /** @brief Scans `root` for .h files (missing/empty root -> empty index). */
  explicit HeaderIndex(const std::filesystem::path &root) {
    if (root.empty() || !std::filesystem::exists(root))
      return;
    std::error_code ec;
    auto it = std::filesystem::recursive_directory_iterator(
        root, std::filesystem::directory_options::skip_permission_denied, ec);
    auto end = std::filesystem::recursive_directory_iterator();
    for (; !ec && it != end; it.increment(ec)) {
      const std::filesystem::path &p = it->path();
      std::error_code fileEc;
      if (!it->is_regular_file(fileEc) || fileEc)
        continue;
      std::string ext = p.extension().string();
      if (ext == ".h")
        _byBasename[p.filename().string()].push_back(p.parent_path());
    }
  }

  /** @brief Directories containing a file named `basename`, or nullptr if none. */
  const std::vector<std::filesystem::path> *find(const std::string &basename) const {
    auto it = _byBasename.find(basename);
    return it == _byBasename.end() ? nullptr : &it->second;
  }

private:
  std::unordered_map<std::string, std::vector<std::filesystem::path>> _byBasename;
};

/** @brief Extracts the text of every quoted `#include "..."`*/
inline std::vector<std::string> extractQuotedIncludes(const std::filesystem::path &filePath) {
  static const std::regex quoted(R"re(^\s*#\s*include\s*"([^"]+)")re");
  std::vector<std::string> includes;
  std::ifstream in(filePath);
  std::string line;
  while (std::getline(in, line)) {
    std::smatch m;
    if (std::regex_search(line, m, quoted))
      includes.push_back(m[1].str());
  }
  return includes;
}

/**
 * @brief Turns a header hit into the -I root that would make `includeQuote`
 * resolve to it
 *
 * E.g. includeQuote "nested/mytypes.h" against candidateDir
 * "repo/include/nested" rebases to "repo/include". Returns nullopt if
 * candidateDir doesn't structurally end with those components - i.e. the
 * basename match was coincidental (an unrelated file of the same name).
 */
inline std::optional<std::filesystem::path> rebaseToIncludeRoot(std::filesystem::path candidateDir,
                                                                 const std::string &includeQuote) {
  std::filesystem::path quoteDir = std::filesystem::path(includeQuote).parent_path();
  std::vector<std::string> parts;
  for (const std::filesystem::path &part : quoteDir)
    parts.push_back(part.string());
  // quoteDir's components read left-to-right ("a/b"), but they need to be
  // peeled off candidateDir's end innermost-first, i.e. "b" then "a".
  for (auto part = parts.rbegin(); part != parts.rend(); ++part) {
    if (candidateDir.filename() != *part)
      return std::nullopt;
    candidateDir = candidateDir.parent_path();
  }
  return candidateDir;
}

/**
 * @brief Picks the -I directory for one quoted #include out of a
 * HeaderIndex's basename candidates, via `rebaseToIncludeRoot`.
 *
 * Note this is for the case a quoted include does *not* resolve against the
 * including file's own directory. Picks a header with the shortest
 * `sourceDir`-relative path, i.e. structurally nearest the including file
 *
 * @param includeQuote Text of the quoted include, e.g. "sub/foo.h".
 * @param index        Prebuilt index of headers under the tree being searched.
 * @param sourceDir    Directory containing the file that has this #include.
 * @return The directory to pass as -I, or nullopt if no candidate at all.
 */
inline std::optional<std::filesystem::path> resolveIncludeDir(const std::string &includeQuote,
                                                               const HeaderIndex &index,
                                                               const std::filesystem::path &sourceDir) {
  std::string basename = std::filesystem::path(includeQuote).filename().string();
  const std::vector<std::filesystem::path> *candidates = index.find(basename);
  if (!candidates || candidates->empty())
    return std::nullopt;

  std::optional<std::filesystem::path> best;
  std::size_t bestDistance = 0;
  for (const std::filesystem::path &candidate : *candidates) {
    std::optional<std::filesystem::path> root = rebaseToIncludeRoot(candidate, includeQuote);
    if (!root)
      continue;
    std::error_code ec;
    std::filesystem::path rel = std::filesystem::relative(*root, sourceDir, ec);
    std::size_t distance = ec ? std::string::npos : std::distance(rel.begin(), rel.end());
    if (!best || distance < bestDistance) {
      best = root;
      bestDistance = distance;
    }
  }
  return best;
}

/** @brief Resolves every quoted #include in `filePath` to a -I dir via `resolveIncludeDir`, deduplicated, first-seen order. */
inline std::vector<std::string> collectLocalIncludeDirs(const std::filesystem::path &filePath,
                                                         const HeaderIndex &index) {
  std::vector<std::string> dirs;
  for (const std::string &quote : extractQuotedIncludes(filePath)) {
    std::optional<std::filesystem::path> dir =
        resolveIncludeDir(quote, index, filePath.parent_path());
    if (!dir)
      continue;
    std::string s = dir->string();
    if (std::find(dirs.begin(), dirs.end(), s) == dirs.end())
      dirs.push_back(s);
  }
  return dirs;
}
