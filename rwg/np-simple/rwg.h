#ifndef RWG_H
#define RWG_H

#include "socket.h"
#include "user.h"

struct RWG {
  RWG() {}

  void cmd(User *user, args_t &args);
  void exit_cmd(args_t &args);
  void setenv_cmd(args_t &args);
  void printenv_cmd(args_t &args);
};

#endif