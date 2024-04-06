#include "utils.h"

#include <fcntl.h>
#include <string.h>
#include <unistd.h>

void utils::dup(int *fd1, int fd2) {
  if ((*fd1 = ::dup(fd2)) < 0) {
    perror("error (dup)");
    exit(EXIT_FAILURE);
  }
}
void utils::dup2(int fd1, int fd2) {
  if (::dup2(fd1, fd2) < 0) {
    perror("error (dup2)");
    exit(EXIT_FAILURE);
  }
}
void utils::pipe(int *fds) {
  if (::pipe(fds) < 0) {
    perror("error (pipe)");
    exit(EXIT_FAILURE);
  }
}
void utils::close(int fd) {
  if (::close(fd) < 0) {
    perror("error (close)");
    exit(EXIT_FAILURE);
  }
}
void utils::select(int nfds, fd_set &rfds) {
  if (::select(nfds, &rfds, (fd_set *)0, (fd_set *)0, (struct timeval *)0) <
      0) {
    perror("error (select)");
    exit(EXIT_FAILURE);
  }
}
void utils::open(int *fd, const char *file, int mode) {
  if ((*fd = ::open(file, mode)) < 0) {
    perror("error (open)");
    exit(EXIT_FAILURE);
  }
}
void utils::write(int fd, std::string msg) {
  if (::write(fd, msg.c_str(), msg.length()) < 0) {
    perror("error (write)");
    exit(EXIT_FAILURE);
  }
}
void utils::execvp(char **argv) {
  if (::execvp(argv[0], argv) < 0) {
    std::cerr << "Unknown command: [" << argv[0] << "]." << std::endl;
    exit(EXIT_FAILURE);
  }
}

void utils::close_pipe(int *fds) {
  close(fds[0]);
  close(fds[1]);
}

void utils::erase_all(std::string &s, char c) {
  auto iter = s.find(c);

  while (iter != std::string::npos) {
    s.erase(iter);
    iter = s.find(c);
  }
}

char **utils::to_argv(args_t &args) {
  char **argv;
  argv = new char *[args.size() + 1];

  for (size_t i = 0; i < args.size(); i++) {
    argv[i] = strdup(args[i].c_str());
  }
  argv[args.size()] = NULL;
  return argv;
}