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

/**
 * @brief Maps header basenames to every directory under a root tree that
 * contains a file with that name.
 *
 * Built once per pipeline run (over databaseDir) and reused across every file
 * being filtered/transformed, so resolving one file's #includes doesn't
 * re-walk the whole tree.
 */
class HeaderIndex {
public:
  /** @brief Recursively scans `root` for .h files. A missing/empty root leaves the index empty. */
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

  /** @brief Directories under root containing a file named `basename`, or nullptr if none. */
  const std::vector<std::filesystem::path> *find(const std::string &basename) const {
    auto it = _byBasename.find(basename);
    return it == _byBasename.end() ? nullptr : &it->second;
  }

private:
  std::unordered_map<std::string, std::vector<std::filesystem::path>> _byBasename;
};

/** @brief Extracts the filenames named by quoted `#include "..."` directives (angle-bracket includes are assumed to be system/library headers and are skipped). */
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
 * @brief Strips `includeSpec`'s own subdirectory components off the end of
 * `candidateDir` (the directory a matching header actually lives in), giving
 * the -I root that would make `includeSpec` resolve to that header.
 *
 * E.g. includeSpec "nested/mytypes.h" against candidateDir
 * "repo/include/nested" rebases to "repo/include" - the -I flag has to name
 * the directory the quoted path is relative to, not the header's own parent.
 * Returns nullopt if candidateDir doesn't structurally end with those
 * components (the basename match was coincidental).
 */
inline std::optional<std::filesystem::path> rebaseToIncludeRoot(std::filesystem::path candidateDir,
                                                                 const std::string &includeSpec) {
  std::filesystem::path specDir = std::filesystem::path(includeSpec).parent_path();
  std::vector<std::string> parts;
  for (const std::filesystem::path &part : specDir)
    parts.push_back(part.string());
  // specDir's components read left-to-right ("a/b"), but they need to be
  // peeled off candidateDir's end innermost-first, i.e. "b" then "a".
  for (auto part = parts.rbegin(); part != parts.rend(); ++part) {
    if (candidateDir.filename() != *part)
      return std::nullopt;
    candidateDir = candidateDir.parent_path();
  }
  return candidateDir;
}

/**
 * @brief Picks the -I directory for one quoted #include spec out of a
 * HeaderIndex's basename candidates.
 *
 * Candidates are first rebased with `rebaseToIncludeRoot`: for a spec with
 * subdirectory components (e.g. "sub/foo.h") only directories that
 * structurally agree with those components count - anything else is a
 * same-basename false positive (e.g. an unrelated vendored copy) and is
 * dropped rather than risk pointing -I at the wrong tree.
 *
 * TODO(you): once rebasing narrows things down, more than one confident
 * candidate can still remain (e.g. the header spec has no subdirectory at
 * all, or the same relative suffix exists under two different subtrees).
 * This placeholder just takes the first one, ignoring everything else about
 * the include site. Replace it with real disambiguation - e.g. prefer the
 * candidate closest to `sourceDir` (the .c file's own directory) by path
 * distance, or shortest path, or first found; there's no single right
 * answer, pick one and leave a one-line note why. Returning `nullopt`
 * instead (no -I added for this include) is always safe: a skipped include
 * just stays unresolved, same as today's behavior.
 *
 * @param includeSpec Text of the quoted include, e.g. "sub/foo.h".
 * @param index       Prebuilt index of headers under the tree being searched.
 * @param sourceDir   Directory containing the file that has this #include.
 * @return The directory to pass as -I, or nullopt if no confident match.
 */
inline std::optional<std::filesystem::path> resolveIncludeDir(const std::string &includeSpec,
                                                               const HeaderIndex &index,
                                                               const std::filesystem::path &sourceDir) {
  (void)sourceDir;
  std::string basename = std::filesystem::path(includeSpec).filename().string();
  const std::vector<std::filesystem::path> *candidates = index.find(basename);
  if (!candidates || candidates->empty())
    return std::nullopt;

  std::vector<std::filesystem::path> rebased;
  for (const std::filesystem::path &candidate : *candidates) {
    std::optional<std::filesystem::path> root = rebaseToIncludeRoot(candidate, includeSpec);
    if (root)
      rebased.push_back(*root);
  }
  if (rebased.empty())
    return std::nullopt;
  return rebased.front();
}

/**
 * @brief Resolves every quoted #include in `filePath` to a -I directory via
 * `resolveIncludeDir`, deduplicated and in first-seen order.
 */
inline std::vector<std::string> collectLocalIncludeDirs(const std::filesystem::path &filePath,
                                                         const HeaderIndex &index) {
  std::vector<std::string> dirs;
  for (const std::string &spec : extractQuotedIncludes(filePath)) {
    std::optional<std::filesystem::path> dir =
        resolveIncludeDir(spec, index, filePath.parent_path());
    if (!dir)
      continue;
    std::string s = dir->string();
    if (std::find(dirs.begin(), dirs.end(), s) == dirs.end())
      dirs.push_back(s);
  }
  return dirs;
}
