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
    "language": "C",
    "minRepoLoC": "100",
    "projectCount": "5",
    "minNumStars": "1",
}

fileSettings = {
    "csv": "dataset.csv",
    "downloadDir": "database",
    "databaseDir": "database",
}

C_EXTENSIONS = {".c", ".h"}


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


# Require a config file path as the first argument
if len(sys.argv) > 1:
    configFile = sys.argv[1]
else:
    print("No Config File Provided.\nAborting Download")
    sys.exit(1)

# Override defaults with values from the config file if present
if os.path.exists(configFile):
    config = configparser.ConfigParser()
    config.read(configFile)
    for setting in downloadSettings:
        try:
            downloadSettings[setting] = config["Downloading"][setting]
        except KeyError as e:
            print(f"KeyError: {e} On Setting {setting}")
    for setting in fileSettings:
        try:
            fileSettings[setting] = config["File Locations"][setting]
        except KeyError as e:
            print(f"KeyError: {e} On Setting {setting}")

print(f"Settings:\n\t{downloadSettings}\n\t{fileSettings}")

headers = get_auth_headers()

if os.path.exists(fileSettings["csv"]):
    with open(fileSettings["csv"], newline="") as csv_file:
        reader = csv.DictReader(csv_file)
        i = 0
        total_files = 0
        start = time.time()
        for row in reader:
            # Stop once we've downloaded the requested number of repos
            if i >= int(downloadSettings["projectCount"]):
                break
            # Skip rows that don't match the language/size/stars filters
            if not (
                row["language"] == downloadSettings["language"]
                and int(row["size"]) >= int(downloadSettings["minRepoLoC"])
                and int(row["stars"]) >= int(downloadSettings["minNumStars"])
            ):
                print(f"Skipping {row['repository']} - Does Not Meet Criteria")
                continue

            location = os.path.join(fileSettings["downloadDir"], row["repository"])
            if os.path.exists(location):
                print(f"{location} already exists")
                i += 1
                continue

            i += 1
            print(f"Downloading .c/.h from {row['repository']}...")
            count = download_c_files(
                row["repository"], fileSettings["downloadDir"], headers
            )
            if count > 0:
                print(f"  Extracted {count} file(s)")
                total_files += count
            else:
                print(f"  No .c/.h files found or download failed")

        end = time.time()
        print(f"Total: {total_files} files from {i} repos in {end - start:.2f}s")
