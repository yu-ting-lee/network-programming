#ifndef RWG_H
#define RWG_H

#include "socket.h"
#include "user.h"

constexpr int MAX_CLIENT = 30;

struct UserPipe {
  UserPipe(int send, int recv, int *fds) : send(send), recv(recv), fds(fds) {}
  int send, recv;
  int *fds;
};

struct RWG {
  RWG() { users.fill(nullptr); }

  void cmd(User *user, args_t &args, std::string line);
  void exit_cmd(User *user, args_t &args, fd_set &afds);
  void yell_cmd(User *user, args_t &args, std::string line);
  void tell_cmd(User *user, args_t &args, std::string line);
  void name_cmd(User *user, args_t &args);
  void who_cmd(User *user, args_t &args);
  void setenv_cmd(User *user, args_t &args);
  void printenv_cmd(args_t &args);

  User *search_user(int fd);
  void login(User *user);
  void clear_upipe(User *user);
  void rollback_env();
  void backup_env(User *user);
  void rollback_fd();
  void backup_fd();

  int *search_upipe(int send, int recv);
  void check_upipe_input(User *user, cmds_t &cmds, std::string line);
  void check_upipe_output(User *user, cmds_t &cmds, std::string line);
  void broadcast(std::string msg);

  std::vector<Env> envs;
  std::vector<UserPipe> upipes;
  std::array<User *, MAX_CLIENT> users;
  std::array<int, 3> sfds;
};

#endif