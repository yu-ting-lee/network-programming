#include "rwg.h"

#include <fcntl.h>
#include <unistd.h>

#include <algorithm>

#include "utils.h"

void RWG::backup_env(User *user) {
  envs.clear();
  for (auto &e : user->envs) {
    char *env = getenv(e.key.c_str());
    envs.emplace_back(e.key, (env != nullptr) ? env : "");
    setenv(e.key.c_str(), e.val.c_str(), 1);
  }
}
void RWG::rollback_env() {
  for (auto &e : envs) {
    setenv(e.key.c_str(), e.val.c_str(), 1);
    if (e.val.empty()) {
      unsetenv(e.key.c_str());
    }
  }
}

void RWG::backup_fd() {
  for (int i = 0; i < 3; i++) {
    utils::dup(&sfds[i], i);
  }
}
void RWG::rollback_fd() {
  for (int i = 0; i < 3; i++) {
    utils::dup2(sfds[i], i);
    utils::close(sfds[i]);
  }
}

void RWG::login(User *user) {
  auto iter = std::find(users.begin(), users.end(), nullptr);
  if (iter != users.end()) {
    user->id = iter - users.begin() + 1;
    users[user->id - 1] = user;
    std::string msg =
        ("****************************************\n"
         "** Welcome to the information server. **\n"
         "****************************************\n");
    utils::write(user->fd, msg);
    msg = "*** User '(no name)' entered from " + user->addr + ". ***\n";
    broadcast(msg);
    utils::write(user->fd, "% ");
  }
}

void RWG::check_upipe_input(User *user, cmds_t &cmds, std::string line) {
  if (cmds.empty() || cmds.front().size() < 2) {
    return;
  }
  args_t &args = cmds.front();
  auto pred = [](const std::string &a) {
    return (a.length() >= 2 && a[0] == '<');
  };
  auto iter = std::find_if(args.begin() + 1, args.end(), pred);
  if (iter != args.end()) {
    std::string s = (*iter).substr(1);
    std::string r = std::to_string(user->id);
    std::string msg;
    args.erase(iter);

    if (!std::all_of(s.begin(), s.end(), isdigit)) {
      std::cerr << "usage: <command> <<user-id>" << std::endl;
      cmds.clear();
      return;
    }
    int send = std::stoi(s) - 1;
    if (send >= MAX_CLIENT || users[send] == nullptr) {
      std::cout << "*** Error: user #" << s << " does not exist yet. ***\n";
      utils::open(&user->dfds[0], "/dev/null", O_RDONLY);
      return;
    }
    if ((user->ufds[0] = search_upipe(send + 1, user->id)) == nullptr) {
      std::cout << "*** Error: the pipe #" << s << "->#" << r
                << " does not exist yet. ***\n";
      utils::open(&user->dfds[0], "/dev/null", O_RDONLY);
      return;
    }
    msg = ("*** " + user->name + " (#" + r + ") just received from " +
           users[send]->name + " (#" + s + ") by '" + line + "' ***\n");
    broadcast(msg);
  }
}

void RWG::check_upipe_output(User *user, cmds_t &cmds, std::string line) {
  if (cmds.empty() || cmds.back().size() < 2) {
    return;
  }
  args_t &args = cmds.back();
  auto pred = [](const std::string &a) {
    return (a.length() >= 2 && a[0] == '>');
  };
  auto iter = std::find_if(args.begin() + 1, args.end(), pred);
  if (iter != args.end()) {
    std::string r = (*iter).substr(1);
    std::string s = std::to_string(user->id);
    std::string msg;
    args.erase(iter);

    if (!std::all_of(r.begin(), r.end(), isdigit)) {
      std::cerr << "usage: <command> ><user-id>" << std::endl;
      cmds.clear();
      return;
    }
    int recv = std::stoi(r) - 1;
    if (recv >= MAX_CLIENT || users[recv] == nullptr) {
      std::cout << "*** Error: user #" << r << " does not exist yet. ***\n";
      utils::open(&user->dfds[1], "/dev/null", O_WRONLY);
      return;
    }
    if (search_upipe(user->id, recv + 1) != nullptr) {
      std::cout << "*** Error: the pipe #" << s << "->#" << r
                << " already exists. ***\n";
      utils::open(&user->dfds[1], "/dev/null", O_WRONLY);
      return;
    }
    int *fds = new int[2];
    utils::pipe(fds);
    upipes.emplace_back(user->id, recv + 1, fds);
    user->ufds[1] = fds;

    msg = ("*** " + user->name + " (#" + s + ") just piped '" + line + "' to " +
           users[recv]->name + " (#" + r + ") ***\n");
    broadcast(msg);
  }
}

User *RWG::search_user(int fd) {
  auto pred = [fd](const User *u) { return (u != nullptr && u->fd == fd); };
  auto iter = std::find_if(users.begin(), users.end(), pred);
  return (iter != users.end()) ? *iter : nullptr;
}

int *RWG::search_upipe(int send, int recv) {
  auto pred = [send, recv](const UserPipe &u) {
    return (u.send == send && u.recv == recv);
  };
  auto iter = std::find_if(upipes.begin(), upipes.end(), pred);
  return (iter != upipes.end()) ? iter->fds : nullptr;
}

void RWG::clear_upipe(User *user) {
  auto pred = [user](const UserPipe &u) { return u.fds == user->ufds[0]; };
  auto iter = std::find_if(upipes.begin(), upipes.end(), pred);

  if (iter != upipes.end()) {
    delete iter->fds;
    upipes.erase(iter);
  }
}

void RWG::broadcast(std::string msg) {
  for (auto &u : users) {
    if (u != nullptr) {
      utils::write(u->fd, msg);
    }
  }
}

void RWG::printenv_cmd(args_t &args) {
  if (args.size() != 2) {
    std::cerr << "usage: printenv <key>" << std::endl;
    return;
  }
  char *env = getenv(args[1].c_str());
  if (env != nullptr) {
    std::cout << env << std::endl;
  }
}

void RWG::setenv_cmd(User *user, args_t &args) {
  if (args.size() != 3) {
    std::cerr << "usage: setenv <key> <value>" << std::endl;
    return;
  }
  char *env = getenv(args[1].c_str());
  envs.emplace_back(args[1], (env != nullptr) ? env : "");
  user->envs.emplace_back(args[1], args[2]);
}

void RWG::exit_cmd(User *user, args_t &args, fd_set &afds) {
  if (args.size() != 1) {
    std::cerr << "usage: exit" << std::endl;
    return;
  }
  utils::close(user->fd);
  FD_CLR(user->fd, &afds);

  for (auto iter = upipes.begin(); iter != upipes.end();) {
    if (iter->send != user->id && iter->recv != user->id) {
      iter++;
    } else {
      utils::close_pipe(iter->fds);
      delete iter->fds;
      iter = upipes.erase(iter);
    }
  }
  users[user->id - 1] = nullptr;
  std::string msg = "*** User '" + user->name + "' left. ***\n";
  broadcast(msg);
  delete user;
}

void RWG::who_cmd(User *user, args_t &args) {
  if (args.size() != 1) {
    std::cerr << "usage: who" << std::endl;
    return;
  }
  std::string msg = "<ID>\t<nickname>\t<IP:port>\t<indicate me>\n";
  for (auto &u : users) {
    if (u != nullptr) {
      msg += std::to_string(u->id) + "\t" + u->name + "\t" + u->addr;
      msg += (u == user) ? "\t<-me\n" : "\n";
    }
  }
  std::cout << msg;
}

void RWG::name_cmd(User *user, args_t &args) {
  if (args.size() != 2) {
    std::cerr << "usage: name <new-name>" << std::endl;
    return;
  }
  auto pred = [args](const User *u) {
    return (u != nullptr && u->name == args[1]);
  };
  if (std::find_if(users.begin(), users.end(), pred) != users.end()) {
    std::cout << "*** User '" + args[1] + "' already exists. ***\n";
    return;
  }
  user->name = args[1];
  std::string msg =
      "*** User from " + user->addr + " is named '" + args[1] + "'. ***\n";
  broadcast(msg);
}

void RWG::yell_cmd(User *user, args_t &args, std::string line) {
  if (args.size() < 2) {
    std::cerr << "usage: yell <message>" << std::endl;
    return;
  }
  line.erase(0, line.find(args[1]));
  std::string msg = "*** " + user->name + " yelled ***: " + line + "\n";
  broadcast(msg);
}

void RWG::tell_cmd(User *user, args_t &args, std::string line) {
  if (args.size() < 3 ||
      !std::all_of(args[1].begin(), args[1].end(), isdigit)) {
    std::cerr << "usage: tell <user-id> <message>" << std::endl;
    return;
  }
  int recv = std::stoi(args[1]) - 1;
  if (users[recv] == nullptr) {
    std::cout << "*** Error: user #" + args[1] + " does not exist yet. ***\n";
    return;
  }
  line.erase(0, line.find(args[2]));
  std::string msg = "*** " + user->name + " told you ***: " + line + "\n";
  utils::write(users[recv]->fd, msg);
}

void RWG::cmd(User *user, args_t &args, std::string line) {
  user->dfds.fill(-1);
  user->rfds.fill(-1);
  user->nfds.fill(nullptr);
  user->ufds.fill(nullptr);

  cmds_t cmds;
  auto iter = std::find(args.begin(), args.end(), "|");

  while (iter != args.end()) {
    cmds.push_back(args_t(args.begin(), iter));
    args.erase(args.begin(), ++iter);
    iter = std::find(args.begin(), args.end(), "|");
  }
  cmds.push_back(args);

  check_upipe_input(user, cmds, line);
  check_upipe_output(user, cmds, line);
  user->check_redirect(cmds);
  user->check_npipe(cmds);

  user->nprocs = cmds.size();
  int fds[cmds.size() - 1][2];
  pid_t child_pid;

  for (size_t i = 0; i < cmds.size(); i++) {
    if (i != cmds.size() - 1) {
      utils::pipe(fds[i]);
    }
    while ((child_pid = fork()) < 0) {
      user->waitpid();
    }
    if (child_pid == 0) { /* child process */
      if (i != 0) {
        utils::dup2(fds[i - 1][0], STDIN_FILENO);
        utils::close_pipe(fds[i - 1]);
      } else {
        user->set_upipe_input();
        user->set_npipe_input();
      }
      if (i != cmds.size() - 1) {
        utils::dup2(fds[i][1], STDOUT_FILENO);
        utils::close_pipe(fds[i]);
      } else {
        user->set_upipe_output();
        user->set_npipe_output();
        user->set_redirect();
      }
      user->execvp(cmds[i]);

    } else { /* parent process */
      if (i != 0) {
        utils::close_pipe(fds[i - 1]);
      } else {
        user->close_upipe_input();
        user->close_npipe_input();
      }
    }
  }
  while (user->nprocs > 0) {
    user->waitpid();
  }
}