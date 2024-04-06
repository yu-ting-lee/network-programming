#include "utils.h"

#include <unistd.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

void utils::dup2(int fd1, int fd2) {
  if (::dup2(fd1, fd2) < 0) {
    perror("error (dup2)");
    exit(EXIT_FAILURE);
  }
}
void utils::setenv(std::string key, std::string val) {
  if (::setenv(key.c_str(), val.c_str(), 1) < 0) {
    perror("error (setenv)");
    exit(EXIT_FAILURE);
  }
}
void utils::split(args_t &args, std::string s, std::string re) {
  boost::split(args, s, boost::is_any_of(re), boost::token_compress_on);
}

void utils::execvp(std::string cgi) {
  char *argv[] = {strdup(cgi.c_str()), NULL};
  if (::execvp(argv[0], argv) < 0) {
    perror("error (execvp)");
    exit(EXIT_FAILURE);
  }
}
