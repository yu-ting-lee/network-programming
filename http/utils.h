#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <vector>

typedef std::vector<std::string> args_t;

namespace utils {

void dup2(int fd1, int fd2);
void setenv(std::string key, std::string val);
void split(args_t &ss, std::string s, std::string re);
void execvp(std::string cgi);

}  // namespace utils

#endif