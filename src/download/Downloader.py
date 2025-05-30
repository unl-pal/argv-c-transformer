import csv
import os
import requests
from git import Repo, RemoteProgress, GitError
import tempfile
import time

start = time.time()

if (os.path.exists("c_repos.csv")):
    os.remove("c_repos.csv")

with open('dataset.csv', newline='') as csv_file:
    with open('c_repos.csv', 'w', newline='') as new_file:
        writer = csv.writer(new_file)
        reader = csv.DictReader(csv_file)
        column_names = reader.fieldnames
        print(column_names)
        writer.writerow(column_names)
        i = 0
        for row in reader:
            if (i >= 5):
                break
            if (row['language'] == 'C'):
                repo_url = f"https://github.com/{row["repository"]}"
                if requests.head(repo_url).status_code != 200:
                    continue
                # repo_url = "https://github.com/" + row["repository"] + "/archive/master.zip"
                # repo_url = "git@github.com:" + row["repository"] + ".git"
                location = "database/" + row["repository"]
                i += 1
                if (not os.path.exists(location)):
                    try:
                        # TODO ZIPS
                    #     print(f"Downloading: {row['repository']}")
                    #     response = requests.get(repo_url+"/archive/master.zip", stream=True)
                    #     response.raise_for_status()
                    #
                    #     with tempfile.NamedTemporaryFile(delete=False) as tmp_file:
                    #         print(f"Saving Temp File: {tmp_file.name}")
                    #         for chunk in response.iter_content():
                    #             tmp_file.write(chunk)
                    #         temp_file_path = tmp_file.name
                    #     print(f"Downloaded to: {temp_file_path}")
                    #
                    #     print(f"Extraction Dir: {location}")
                    #     if not os.path.exists(location):
                    #         os.makedirs(location, exist_ok=True)
                    #         with zipfile.ZipFile(temp_file_path, 'r') as zip_ref:
                    #             zip_ref.extractall(location)
                    #
                    #         print(f"Extracted: {temp_file_path} to: {location}")
                    #
                    # except requests.exceptions.RequestException as e:
                    #     print(f"Error: Downloading: {e}")
                    #
                    # except zipfile.BadZipFile:
                    #     print(f"Error: Download File is Not Valid Zip File")
                    #
                    # except Exception as e:
                    #     print(f"Error: Unexpected Error Occured: {e}")
                    #
                    # finally:
                    #     if 'temp_file_path' in locals() and os.path.exists(location):
                    #         print(f"Deleting Temporary File: {temp_file_path}")
                    #         os.remove(temp_file_path)
            # TODO decide between zips and clones

                    # repo = Repo.clone_from(repo_url, location, multi_options=["--depth=1"])
                        # requests.urllib3 #thread safe pooling
            # TODO CLONES
                        print(f"Cloning: {row["repository"]}")
                        repo = Repo.clone_from(repo_url+".git", location, progress=RemoteProgress(), multi_options=["--depth=1"])
                        writer.writerow(row.values())
                        print(f"Finished")
                    except GitError as e:
                        print(f"Error: {e}")
            # TODO CLONES
                    # except requests.exceptions.RequestException:
                    #     print(f"No Git Repo: {row["repository"]}")
                        # repo = repo.clone_from()
end = time.time()

print(f"Total Time: {end - start}")
