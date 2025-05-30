import csv
import os
import requests
from git import Repo, RemoteProgress, GitError
import tempfile
import time
import configparser

start = time.time()

# default settings
settings = {
    'language': 'C',
    'minRepoLoC': "100",
    'projectCount': "5",
    'minNumStars': "1"
}

if (os.path.exists("properties.config")):
    config = configparser.ConfigParser()
    config.read('properties.config')
    # print(config.sections())
    for setting in settings:
        try:
            settings[setting] = config['Downloading'][setting]
        except KeyError as e:
            print(f"KeyError: {e} On Setting {setting}")

print(f"Settings: {settings}")

if (os.path.exists("c_repos.csv")):
    os.remove("c_repos.csv")

with open('dataset.csv', newline='') as csv_file:
    with open('c_repos.csv', 'w', newline='') as new_file:
        writer = csv.writer(new_file)
        reader = csv.DictReader(csv_file)
        column_names = reader.fieldnames
        # print(column_names)
        writer.writerow(column_names)
        i = 0
        for row in reader:
            if (i >= (int)(settings['projectCount'])):
                break
            if (row['language'] == settings['language'] 
                    and (int)(row['size']) >= (int)(settings['minRepoLoC'])):
                repo_url = f"https://github.com/{row["repository"]}"
                if requests.head(repo_url).status_code != 200:
                    continue
                location = "database/" + row["repository"]
                i += 1
                if (not os.path.exists(location)):
                    try:
                        print(f"Cloning: {row["repository"]}")
                        repo = Repo.clone_from(repo_url+".git", location, progress=RemoteProgress.update(0, 0, 0), multi_options=["--depth=1"])
                        writer.writerow(row.values())
                        print(f"Finished")
                    except GitError as e:
                        print(f"Error: {e}")

end = time.time()

print(f"Total Time: {end - start}")
