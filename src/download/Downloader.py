# SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
#
# SPDX-License-Identifier: Apache-2.0

import sys
import csv
import os
import io
import tarfile
import requests
import time
import configparser

# Defaults used when a setting is missing from the config file
downloadSettings = {
    "projectCount": "5",
    # Optional: a single "owner/name" repo to download, bypassing the CSV
    # index and all [Downloader] filter criteria entirely
    "repo": "",
}

fileSettings = {
    "csv": "repos.csv",
    "databaseDir": "database",
}

C_EXTENSIONS = {".c", ".h"}


def row_matches_filters(row, filters):
    """
    Check whether a CSV row satisfies every (column, value) pair in `filters`.

    For each filter:
      - If it's numeric treat filter as min
      - Otherwise treat the filter as a selection

    row: dict (column name -> string value)
    filters: dict of column name -> configured filter value
    Returns True if the row passes every filter, False otherwise.
    """
    for col, val in filters.items():
        this_value = row.get(col)
        try:
            if float(this_value) < float(val):
                return False
        except ValueError:
            options = val.split(",")
            if this_value not in options:
                return False
    return True


def get_auth_headers():
    """Build auth headers from GITHUB_TOKEN if available."""
    token = os.environ.get("GITHUB_TOKEN")
    if token:
        return {"Authorization": f"token {token}"}
    print(
        "Warning: GITHUB_TOKEN not set — using unauthenticated API (60 req/hour limit)"
    )
    return {}


def download_c_files(repo, dest_dir, headers):
    """
    Download a repo tarball and extract only .c/.h files.
    Rename using dest_dir/{owner}/{repo}/ structure.
    Returns the number of files extracted, or 0 on failure.
    """
    owner, name = repo.split("/")
    tarball_url = f"https://api.github.com/repos/{repo}/tarball"
    response = requests.get(tarball_url, headers=headers)

    if response.status_code == 404:
        print(f"Skipping {repo} - Not Accessible (404)")
        return 0
    if response.status_code == 403:
        print(f"Skipping {repo} - Rate Limited or Forbidden (403)")
        return 0
    if response.status_code != 200:
        print(f"Skipping {repo} - HTTP {response.status_code}")
        return 0

    count = 0
    archive_bytes = io.BytesIO(response.content)
    with tarfile.open(fileobj=archive_bytes, mode="r:gz") as tar:
        for member in tar.getmembers():
            if not member.isfile():
                continue
            _, ext = os.path.splitext(member.name)
            if ext not in C_EXTENSIONS:
                continue

            # Strip the GitHub top-level dir (owner-repo-sha/)
            parts = member.name.split("/", 1)
            if len(parts) < 2:
                continue
            rel_path = parts[1]

            out_path = os.path.join(dest_dir, owner, name, rel_path)
            if not os.path.realpath(out_path).startswith(os.path.realpath(dest_dir)):
                continue
            os.makedirs(os.path.dirname(out_path), exist_ok=True)

            src = tar.extractfile(member)
            if src is None:
                continue
            with src, open(out_path, "wb") as dst:
                dst.write(src.read())
            count += 1

    return count


def download_repos(rows, dest_dir, headers, limit=None, filters=None):
    """
    Download .c/.h files for each repo named in `rows` (dicts with a
    "repository" column). If `filters`/`limit` are given, rows are skipped
    when they don't match and iteration stops once `limit` repos are
    downloaded; otherwise every row is downloaded unconditionally.
    """
    i = 0
    total_files = 0
    start = time.time()
    for row in rows:
        if limit is not None and i >= limit:
            break
        if filters and not row_matches_filters(row, filters):
            print(f"Skipping {row['repository']} - Does Not Meet Criteria")
            continue

        location = os.path.join(dest_dir, row["repository"])
        if os.path.exists(location):
            print(f"{location} already exists")
            i += 1
            continue

        i += 1
        print(f"Downloading .c/.h from {row['repository']}...")
        count = download_c_files(row["repository"], dest_dir, headers)
        if count > 0:
            print(f"  Extracted {count} file(s)")
            total_files += count
        else:
            print(f"  No .c/.h files found or download failed")

    end = time.time()
    print(f"Total: {total_files} files from {i} repos in {end - start:.2f}s")


# First argument is either a CSV (plain list of repos, downloaded
# unconditionally with no filtering) or a .config file
if len(sys.argv) > 1:
    inputFile = sys.argv[1]
else:
    print("No CSV or Config File Provided.\nAborting Download")
    sys.exit(1)

headers = get_auth_headers()

if inputFile.lower().endswith(".csv"):
    if not os.path.exists(inputFile):
        print(f"{inputFile} not found.\nAborting Download")
        sys.exit(1)
    with open(inputFile, newline="") as csv_file:
        reader = csv.DictReader(csv_file)
        if "repository" not in (reader.fieldnames or []):
            print(
                f"CSV {inputFile} has no 'repository' column "
                f"(columns: {reader.fieldnames}).\nAborting Download"
            )
            sys.exit(1)
        download_repos(reader, fileSettings["databaseDir"], headers)
    sys.exit(0)

configFile = inputFile

# Override defaults with values from the config file if present
if os.path.exists(configFile):
    config = configparser.ConfigParser(inline_comment_prefixes=("#",))
    config.read(configFile)
    for setting in downloadSettings:
        try:
            downloadSettings[setting] = config["Downloader"][setting]
        except KeyError as e:
            print(f"KeyError: {e} On Setting {setting}")
    for setting in fileSettings:
        try:
            fileSettings[setting] = config["File Locations"][setting]
        except KeyError as e:
            print(f"KeyError: {e} On Setting {setting}")
    # Everything else in [Downloader] besides the known settings above is
    # treated as a CSV column filter (see row_matches_filters). configparser
    # lowercases option names, so compare against lowercased setting names.
    known = {config.optionxform(s) for s in downloadSettings}
    filters = (
        {k: v for k, v in config["Downloader"].items() if k not in known}
        if config.has_section("Downloader")
        else {}
    )
else:
    filters = {}

print(f"Settings:\n\t{downloadSettings}\n\t{fileSettings}\n\tFilters: {filters}")

if downloadSettings["repo"]:
    repo = downloadSettings["repo"]
    location = os.path.join(fileSettings["databaseDir"], repo)
    if os.path.exists(location):
        print(f"{location} already exists")
    else:
        print(f"Downloading .c/.h from {repo}...")
        count = download_c_files(repo, fileSettings["databaseDir"], headers)
        if count > 0:
            print(f"  Extracted {count} file(s)")
        else:
            print(f"  No .c/.h files found or download failed")
    sys.exit(0)

if os.path.exists(fileSettings["csv"]):
    with open(fileSettings["csv"], newline="") as csv_file:
        reader = csv.DictReader(csv_file)

        unknown_keys = [k for k in filters if k not in (reader.fieldnames or [])]
        if unknown_keys:
            print(
                f"Filter key(s) {unknown_keys} not found in CSV columns "
                f"{reader.fieldnames}.\nAborting Download"
            )
            sys.exit(1)

        download_repos(
            reader,
            fileSettings["databaseDir"],
            headers,
            limit=int(downloadSettings["projectCount"]),
            filters=filters,
        )
