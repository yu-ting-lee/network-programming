#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <vector>

typedef std::vector<std::string> args_t;

namespace utils {

void dup(int *fd1, int fd2);
void dup2(int fd1, int fd2);
void pipe(int *fds);
void close(int fd);

char **to_argv(args_t &args);
void erase_all(std::string &s, char c);
void close_pipe(int *fds);

void select(int nfds, fd_set &rfds);
void open(int *fd, const char *file, int mode);
void write(int fd, std::string msg);
void execvp(char **argv);

}  // namespace utils

#endif