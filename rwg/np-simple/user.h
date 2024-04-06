#ifndef USER_H
#define USER_H

#include <array>
#include <iostream>
#include <vector>

typedef std::vector<std::string> args_t;
typedef std::vector<args_t> cmds_t;

struct NumberPipe {
  NumberPipe(int val, int *fds) : val(val), fds(fds) {}
  int val;
  int *fds;
};

struct User {
  User() {}

  int *search_npipe(int val);
  void check_npipe(cmds_t &cmds);
  void clear_npipe();
  void count_npipe();
  void close_npipe_input();
  void set_npipe_output();
  void set_npipe_input();

  void check_redirect(cmds_t &cmds);
  void set_redirect();

  void execvp(args_t &args);
  void waitpid();

  std::array<int, 1> rfds;
  std::array<int *, 3> nfds;
  std::vector<NumberPipe> npipes;
  int nprocs;
};

#endif