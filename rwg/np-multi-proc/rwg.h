#ifndef RWG_H
#define RWG_H

#include <semaphore.h>
#include <string.h>

#include "socket.h"
#include "user.h"

constexpr int MAX_MSGLEN = 1024, MAX_CLIENT = 30;
constexpr int READ = 0, OPEN = 1;
constexpr char PATH[] = "user_pipe/";

struct UserPipe {
  UserPipe(std::string file) : file(file) {}
  std::string file;
  int fd;
};

struct RWG_MSG {
  RWG_MSG(int type, std::string msg) : type(type) {
    strcpy(this->msg, msg.c_str());
  };
  int type;
  char msg[MAX_MSGLEN];
};

struct RWG {
  RWG() { upipes.fill(nullptr); };

  void cmd(User *user, args_t &args, std::string line);
  void exit_cmd(args_t &args);
  void yell_cmd(args_t &args, std::string line);
  void tell_cmd(args_t &args, std::string line);
  void name_cmd(args_t &args);
  void who_cmd(args_t &args);
  void setenv_cmd(args_t &args);
  void printenv_cmd(args_t &args);

  void waitpid();
  void login(Info &info);
  void shm_attach();
  void shm_detach();

  void del_upipe(UserPipe *&upipe);
  void check_upipe_input(User *user, cmds_t &cmds, std::string line);
  void check_upipe_output(User *user, cmds_t &cmds, std::string line);
  void broadcast(std::string msg);

  inline std::string get_name(int id) { return info[id - 1].name; }
  inline std::string get_addr(int id) { return info[id - 1].addr; }
  inline std::string get_file(int s, int r) {
    return PATH + std::to_string(info[s].pid) + std::to_string(info[r].pid);
  }
  int id, nprocs;
  std::array<UserPipe *, MAX_CLIENT> upipes;
  int msg_id, info_id, lock_id, stat_id;
  RWG_MSG *msg;
  Info *info;
  sem_t *lock;
  bool *stat;
};

#endif