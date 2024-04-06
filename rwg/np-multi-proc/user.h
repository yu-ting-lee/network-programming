#ifndef USER_H
#define USER_H

#include <array>
#include <iostream>
#include <vector>

#include "socket.h"

typedef std::vector<std::string> args_t;
typedef std::vector<args_t> cmds_t;

struct NumberPipe {
  NumberPipe(int val, int *fds) : val(val), fds(fds) {}
  int val;
  int *fds;
};

struct Info {
  Info(Client &cli);
  char name[20], addr[30];
  pid_t pid;
};

struct User {
  User() {}

  void close_upipe_input();
  void close_upipe_output();
  void set_upipe_input();
  void set_upipe_output();

  int *search_npipe(int val);
  void check_npipe(cmds_t &cmds);
  void clear_npipe();
  void count_npipe();
  void close_npipe_input();
  void set_npipe_input();
  void set_npipe_output();

  void check_redirect(cmds_t &cmds);
  void set_redirect();

  void execvp(args_t &args);
  void waitpid();

  std::array<int, 1> rfds;
  std::array<int, 2> dfds;
  std::array<int, 2> ufds;
  std::array<int *, 3> nfds;
  std::vector<NumberPipe> npipes;
  std::string file;
  int nprocs;
};

#endif