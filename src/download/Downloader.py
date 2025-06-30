import sys
import csv
import os
import requests
from git import Repo, RemoteProgress, GitError
import tempfile
import time
import configparser

# default settings
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

if (sys.argv.__sizeof__() > 1):
    configFile = sys.argv[1]
else:
    print(f"No Config File Provided.\nAborting Download")

if (os.path.exists(configFile)):
    config = configparser.ConfigParser()
    config.read(configFile)
    # print(config.sections())
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

if (os.path.exists(fileSettings['csv'])):
    with open(fileSettings['csv'], newline='') as csv_file:
        reader = csv.DictReader(csv_file)
        column_names = reader.fieldnames
        i = 0
        start = time.time()
        for row in reader:
            if (i >= (int)(downloadSettings['projectCount'])):
                break
            if (row['language'] == downloadSettings['language'] 
                    and (int)(row['size']) >= (int)(downloadSettings['minRepoLoC'])
                    and (int)(row['stars']) >= (int)(downloadSettings['minNumStars'])):
                repo_url = f"https://github.com/{row["repository"]}"
                if requests.head(repo_url).status_code != 200:
                    continue
                location = os.path.join(fileSettings['downloadDir'], row["repository"])
                i += 1
                print(f"Attempting to Clone {location}")
                if (not os.path.exists(location)):
                    try:
                        print(f"Cloning: {row["repository"]}")
                        repo = Repo.clone_from(repo_url+".git", location, progress=RemoteProgress.update(0, 0, 0), multi_options=["--depth=1"])
                        print(f"Finished")
                    except GitError as e:
                        print(f"Error: {e}")
                else:
                    print(f"{location} already exists")

        end = time.time()

print(f"Total Time: {end - start}")
