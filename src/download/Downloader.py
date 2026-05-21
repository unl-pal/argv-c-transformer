import sys
import csv
import os
import requests
from git import Repo, GitError
import time
import configparser

# Defaults used when a setting is missing from the config file
downloadSettings = {
    'language': 'C',
    'minRepoLoC': "100",
    'projectCount': "5",
    'minNumStars': "1"
}

fileSettings = {
    'csv': 'dataset.csv',
    'downloadDir': 'database',
    'databaseDir': 'database'
}

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
            downloadSettings[setting] = config['Downloading'][setting]
        except KeyError as e:
            print(f"KeyError: {e} On Setting {setting}")
    for setting in fileSettings:
        try:
            fileSettings[setting] = config['File Locations'][setting]
        except KeyError as e:
            print(f"KeyError: {e} On Setting {setting}")

print(f"Settings:\n\t{downloadSettings}\n\t{fileSettings}")

if os.path.exists(fileSettings['csv']):
    with open(fileSettings['csv'], newline='') as csv_file:
        reader = csv.DictReader(csv_file)
        i = 0
        start = time.time()
        for row in reader:
            # Stop once we've cloned the requested number of repos
            if i >= int(downloadSettings['projectCount']):
                break
            # Skip rows that don't match the language/size/stars filters
            if not (row['language'] == downloadSettings['language']
                    and int(row['size']) >= int(downloadSettings['minRepoLoC'])
                    and int(row['stars']) >= int(downloadSettings['minNumStars'])):
                print(f"Skipping {row['repository']} - Does Not Meet Criteria")
                continue

            repo_url = f"https://github.com/{row['repository']}"
            # Skip repos that are no longer accessible
            if requests.head(repo_url).status_code != 200:
                print(f"Skipping {repo_url} - Not Accessible")
                continue

            location = os.path.join(fileSettings['downloadDir'], row['repository'])
            i += 1
            print(f"Attempting to Clone {location}")
            if not os.path.exists(location):
                try:
                    print(f"Cloning: {row['repository']}")
                    # --depth=1 fetches only the latest commit to keep downloads small
                    Repo.clone_from(repo_url + ".git", location, multi_options=["--depth=1"])
                    print("Finished")
                except GitError as e:
                    print(f"Error: {e}")
            else:
                print(f"{location} already exists")

        end = time.time()
        print(f"Total Time: {end - start:.2f}s")
