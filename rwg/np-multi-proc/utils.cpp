#include "utils.h"

#include <fcntl.h>
#include <string.h>
#include <sys/shm.h>
#include <unistd.h>

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
void utils::sigaction(int signo, sighandler_t hdl) {
  struct sigaction sa;
  sa.sa_flags = SA_RESTART;
  sa.sa_handler = hdl;
  sigemptyset(&(sa.sa_mask));

  if (sigaction(signo, &sa, NULL) < 0) {
    perror("error (sigaction)");
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

void utils::sem_init(sem_t *sem) {
  if (::sem_init(sem, 1, 1) < 0) {
    perror("error (sem_init)");
    exit(EXIT_FAILURE);
  }
}
void utils::sem_destroy(sem_t *sem) {
  if (::sem_destroy(sem) < 0) {
    perror("error (sem_destroy)");
    exit(EXIT_FAILURE);
  }
}
void utils::sem_wait(sem_t *sem) {
  if (::sem_wait(sem) < 0) {
    perror("error (sem_wait)");
    exit(EXIT_FAILURE);
  }
}
void utils::sem_post(sem_t *sem) {
  if (::sem_post(sem) < 0) {
    perror("error (sem_post)");
    exit(EXIT_FAILURE);
  }
}

void utils::shmctl(int *id) {
  if (::shmctl(*id, IPC_RMID, NULL) < 0) {
    perror("error (shmctl)");
    exit(EXIT_FAILURE);
  }
}
void utils::shmget(int *id, size_t size) {
  if ((*id = ::shmget(IPC_PRIVATE, size, IPC_CREAT | 0600)) < 0) {
    perror("error (shmget)");
    exit(EXIT_FAILURE);
  }
  std::cout << "shmid: " << *id << std::endl;
}
void utils::shmdt(void *addr) {
  if (::shmdt(addr) < 0) {
    perror("error (shmdt)");
    exit(EXIT_FAILURE);
  }
}
void *utils::shmat(int id) {
  void *addr;
  if ((addr = ::shmat(id, NULL, 0)) == (void *)-1) {
    perror("error (shmat)");
    exit(EXIT_FAILURE);
  }
  return addr;
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