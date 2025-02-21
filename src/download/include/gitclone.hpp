#pragma once

#include <memory>
#include <string>
#include <vector>

class GitClone {
public:
	GitClone();

	GitClone(bool github);

	int cloneAllRepos(const std::vector<std::string> &repoUrls);

	bool cloneRepo(const std::string repoUrl);

	std::vector<std::string> readCsv(std::string csvName);

	void convert2Url(std::vector<std::string> *names);

private:
	std::vector<std::string> _urls;
	std::string _csvName;
};
