#include "include/gitclone.hpp"

#include <git2/checkout.h>
#include <git2/clone.h>
#include <git2/config.h>
#include <git2/deprecated.h>
#include <git2/errors.h>
#include <git2/global.h>
#include <git2/remote.h>
#include <git2/repository.h>
#include <git2/types.h>

#include <iostream>
#include <string>

int myClone(int argc, char *myUrl) {
  int error = 0;
  if (argc == 2) {
    // With a fresh dir this will create a shallow clone of a repo
    git_libgit2_init();
    git_repository *repo = NULL;
    const char *url = myUrl;
    const char *path = "./database/temp";
    /*const git_clone_options *clone_opts = NULL;*/
    /*git_repository_init(&repo, path, 0);*/
    git_clone_options clone_opts = GIT_CLONE_OPTIONS_INIT;
    /*git_clone_init_options(&clone_opts, 0);*/
    clone_opts.fetch_opts.depth = 1;
    error = git_clone(&repo, url, path, &clone_opts);
    /*error = git_clone(&repo, url, path, clone_opts);*/
    /*error = git_clone(&repo, url, path, NULL);*/
    git_repository_free(repo);
    git_libgit2_shutdown();
  }
  else if (argc == 3) {
    // freshly made the temp dir and that is aout the extent of my knowledge
    // will create full clone of the repo
    git_libgit2_init();
    git_repository *repo = NULL;
    const char *url = myUrl;
    const char *path = "./database/temp";
    error = git_clone(&repo, url, path, NULL);
    git_repository_free(repo);
    git_libgit2_shutdown();
  }
  std::cout << error << std::endl;
  std::cout << git_error_last()->message << std::endl;
  return error;
}

int main(int argc, char** argv) {
  if (argc > 1) {
    myClone(argc, argv[1]);
  } else {
    myClone(2, (char*)"https://github.com/unl-pal/argv-transformer.git");
  }
  return 0;
}
