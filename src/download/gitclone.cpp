#include "include/gitclone.hpp"

#include <git2/clone.h>
#include <string>
#include <vector>
#include <iostream>

#include <git2.h>

GitClone::GitClone() {
  _urls = std::vector<std::string>();
}

bool GitClone::cloneRepo(const std::string repoUrl) {
  std::string mine = repoUrl;
  return false;
}

int GitClone::cloneAllRepos(const std::vector<std::string> &repoUrls) {
  int success = 0;
  for (const auto& repo : repoUrls) {
    std::cout << repo.c_str() << std::endl;
    success++;
    /*git_clone(git_repository **out, const char *url, const char *local_path, const git_clone_options *options)*/
  }
  return success;
}

std::vector<std::string> GitClone::readCsv(std::string csvName) {
  _urls.push_back(csvName);
  return _urls;
}

void GitClone::convert2Url(std::vector<std::string> *names) {
  for (auto name : *names) {
    name = std::string("this") += name;
  }

}
