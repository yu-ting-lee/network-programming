#ifndef UTILS_H
#define UTILS_H

#include <array>
#include <iostream>
#include <vector>

typedef std::vector<std::string> args_t;

namespace utils {

void dup2(int fd1, int fd2);
void pipe(int *fds);
void close(int fd);
void execvp(char **argv);

char **to_argv(args_t &args);
void erase_all(std::string &s, char c);
void close_pipe(int *fds);

}  // namespace utils

#endif