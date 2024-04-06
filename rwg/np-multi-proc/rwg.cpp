#include "rwg.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>

#include "utils.h"

void RWG::shm_attach() {
  utils::shmget(&msg_id, sizeof(RWG_MSG));
  utils::shmget(&info_id, sizeof(Info) * MAX_CLIENT);
  utils::shmget(&lock_id, sizeof(sem_t));
  utils::shmget(&stat_id, sizeof(bool) * MAX_CLIENT);

  msg = (RWG_MSG *)utils::shmat(msg_id);
  info = (Info *)utils::shmat(info_id);
  lock = (sem_t *)utils::shmat(lock_id);
  stat = (bool *)utils::shmat(stat_id);

  utils::sem_init(lock);
}

void RWG::shm_detach() {
  utils::sem_destroy(lock);

  utils::shmdt((void *)msg);
  utils::shmdt((void *)info);
  utils::shmdt((void *)lock);
  utils::shmdt((void *)stat);

  utils::shmctl(&msg_id);
  utils::shmctl(&info_id);
  utils::shmctl(&lock_id);
  utils::shmctl(&stat_id);
}

void RWG::login(Info &info) {
  auto iter = std::find(stat, stat + MAX_CLIENT, false);
  if (iter != stat + MAX_CLIENT) {
    id = iter - stat + 1;
    this->info[id - 1] = info;
    stat[id - 1] = true;
    std::string msg =
        ("****************************************\n"
         "** Welcome to the information server. **\n"
         "****************************************\n");
    std::cout << msg;
    msg = "*** User '(no name)' entered from " + get_addr(id) + ". ***\n";
    broadcast(msg);
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
    std::string r = std::to_string(id);
    std::string f, msg;
    args.erase(iter);

    if (!std::all_of(s.begin(), s.end(), isdigit)) {
      std::cerr << "usage: <command> <<user-id>" << std::endl;
      cmds.clear();
      return;
    }
    int send = std::stoi(s) - 1;
    if (send >= MAX_CLIENT || !stat[send]) {
      std::cout << "*** Error: user #" << s << " does not exist yet. ***\n";
      utils::open(&user->dfds[0], "/dev/null", O_RDONLY);
      return;
    }
    if (f = get_file(send, id - 1); access(f.c_str(), F_OK) < 0) {
      std::cout << "*** Error: the pipe #" << s << "->#" << r
                << " does not exist yet. ***\n";
      utils::open(&user->dfds[0], "/dev/null", O_RDONLY);
      del_upipe(upipes[send]);
      return;
    };
    user->ufds[0] = upipes[send]->fd;
    user->file = upipes[send]->file;
    delete upipes[send];
    upipes[send] = nullptr;

    msg = ("*** " + get_name(id) + " (#" + r + ") just received from " +
           get_name(send + 1) + " (#" + s + ") by '" + line + "' ***\n");
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
    std::string s = std::to_string(id);
    std::string f, msg;
    args.erase(iter);

    if (!std::all_of(r.begin(), r.end(), isdigit)) {
      std::cerr << "usage: <command> ><user-id>" << std::endl;
      cmds.clear();
      return;
    }
    int recv = std::stoi(r) - 1;
    if (recv >= MAX_CLIENT || !stat[recv]) {
      std::cout << "*** Error: user #" << r << " does not exist yet. ***\n";
      utils::open(&user->dfds[1], "/dev/null", O_WRONLY);
      return;
    }
    if (f = get_file(id - 1, recv); mknod(f.c_str(), S_IFIFO | 0666, 0) < 0) {
      std::cout << "*** Error: the pipe #" << s << "->#" << r
                << " already exists. ***\n";
      utils::open(&user->dfds[1], "/dev/null", O_WRONLY);
      return;
    }
    utils::sem_wait(lock);
    *this->msg = RWG_MSG(OPEN, s);
    kill(info[recv].pid, SIGUSR1);
    utils::open(&user->ufds[1], f.c_str(), O_WRONLY);

    msg = ("*** " + get_name(id) + " (#" + s + ") just piped '" + line +
           "' to " + get_name(recv + 1) + " (#" + r + ") ***\n");
    broadcast(msg);
  }
}

void RWG::del_upipe(UserPipe *&upipe) {
  if (upipe != nullptr) {
    utils::close(upipe->fd);
    unlink(upipe->file.c_str());
    delete upipe;
    upipe = nullptr;
  }
}

void RWG::broadcast(std::string msg) {
  for (int i = 0; i < MAX_CLIENT; i++) {
    if (stat[i]) {
      utils::sem_wait(lock);
      *this->msg = RWG_MSG(READ, msg);
      kill(info[i].pid, SIGUSR1);
    }
  }
}

void RWG::waitpid() {
  int status;

  while (::waitpid(-1, &status, WNOHANG) > 0) {
    nprocs--;
  }
}

void RWG::printenv_cmd(args_t &args) {
  if (args.size() != 2) {
    std::cerr << "usage: printenv <var>" << std::endl;
    return;
  }
  char *env = getenv(args[1].c_str());
  if (env != nullptr) {
    std::cout << env << std::endl;
  }
}

void RWG::setenv_cmd(args_t &args) {
  if (args.size() != 3) {
    std::cerr << "usage: setenv <var> <value>" << std::endl;
    return;
  }
  if (setenv(args[1].c_str(), args[2].c_str(), 1) < 0) {
    perror("error (setenv)");
    exit(EXIT_FAILURE);
  }
}

void RWG::exit_cmd(args_t &args) {
  if (args.size() != 1) {
    std::cerr << "usage: exit" << std::endl;
    return;
  }
  for (auto &u : upipes) {
    del_upipe(u);
  }
  stat[id - 1] = false;
  std::string msg = "*** User '" + get_name(id) + "' left. ***\n";
  broadcast(msg);
  exit(EXIT_SUCCESS);
}

void RWG::who_cmd(args_t &args) {
  if (args.size() != 1) {
    std::cerr << "usage: who" << std::endl;
    return;
  }
  std::string msg = "<ID>\t<nickname>\t<IP:port>\t<indicate me>\n";
  for (int i = 0; i < MAX_CLIENT; i++) {
    if (stat[i]) {
      msg += std::to_string(i + 1) + "\t" + info[i].name + "\t" + info[i].addr;
      msg += ((i + 1) == id) ? "\t<-me\n" : "\n";
    }
  }
  std::cout << msg;
}

void RWG::name_cmd(args_t &args) {
  if (args.size() != 2) {
    std::cerr << "usage: name <new-name>" << std::endl;
    return;
  }
  for (int i = 0; i < MAX_CLIENT; i++) {
    if (stat[i] && info[i].name == args[1]) {
      std::cout << "*** User '" + args[1] + "' already exists. ***\n";
      return;
    }
  }
  strcpy(info[id - 1].name, args[1].c_str());
  std::string msg =
      "*** User from " + get_addr(id) + " is named '" + args[1] + "'. ***\n";
  broadcast(msg);
}

void RWG::yell_cmd(args_t &args, std::string line) {
  if (args.size() < 2) {
    std::cerr << "usage: yell <message>" << std::endl;
    return;
  }
  line.erase(0, line.find(args[1]));
  std::string msg = "*** " + get_name(id) + " yelled ***: " + line + "\n";
  broadcast(msg);
}

void RWG::tell_cmd(args_t &args, std::string line) {
  if (args.size() < 3 ||
      !std::all_of(args[1].begin(), args[1].end(), isdigit)) {
    std::cerr << "usage: tell <user-id> <message>" << std::endl;
    return;
  }
  int recv = std::stoi(args[1]) - 1;
  if (!stat[recv]) {
    std::cout << "*** Error: user #" + args[1] + " does not exist yet. ***\n";
    return;
  }
  line.erase(0, line.find(args[2]));
  std::string msg = "*** " + get_name(id) + " told you ***: " + line + "\n";
  utils::sem_wait(lock);
  *this->msg = RWG_MSG(READ, msg);
  kill(info[recv].pid, SIGUSR1);
}

void RWG::cmd(User *user, args_t &args, std::string line) {
  user->dfds.fill(-1);
  user->rfds.fill(-1);
  user->nfds.fill(nullptr);
  user->ufds.fill(-1);

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
  user->close_upipe_output();

  while (user->nprocs > 0) {
    user->waitpid();
  }
}