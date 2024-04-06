#ifndef UTILS_H
#define UTILS_H

#include <semaphore.h>
#include <signal.h>

#include <array>
#include <iostream>
#include <vector>

typedef std::vector<std::string> args_t;

namespace utils {

void dup2(int fd1, int fd2);
void pipe(int *fds);
void close(int fd);

char **to_argv(args_t &args);
void erase_all(std::string &s, char c);
void close_pipe(int *fds);

void sigaction(int signo, sighandler_t hdl);
void open(int *fd, const char *file, int mode);
void write(int fd, std::string msg);
void execvp(char **argv);

void sem_init(sem_t *sem);
void sem_destroy(sem_t *sem);
void sem_wait(sem_t *sem);
void sem_post(sem_t *sem);

void shmget(int *id, size_t size);
void shmctl(int *id);
void *shmat(int id);
void shmdt(void *addr);

}  // namespace utils

#endif