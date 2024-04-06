#include "rwg.h"

#include <unistd.h>

#include <algorithm>

#include "utils.h"

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
  exit(EXIT_SUCCESS);
}

void RWG::cmd(User *user, args_t &args) {
  user->rfds.fill(-1);
  user->nfds.fill(nullptr);

  cmds_t cmds;
  auto iter = std::find(args.begin(), args.end(), "|");

  while (iter != args.end()) {
    cmds.push_back(args_t(args.begin(), iter));
    args.erase(args.begin(), ++iter);
    iter = std::find(args.begin(), args.end(), "|");
  }
  cmds.push_back(args);

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
        user->set_npipe_input();
      }
      if (i != cmds.size() - 1) {
        utils::dup2(fds[i][1], STDOUT_FILENO);
        utils::close_pipe(fds[i]);
      } else {
        user->set_npipe_output();
        user->set_redirect();
      }
      user->execvp(cmds[i]);

    } else { /* parent process */
      if (i != 0) {
        utils::close_pipe(fds[i - 1]);
      } else {
        user->close_npipe_input();
      }
    }
  }
  while (user->nprocs > 0) {
    user->waitpid();
  }
}
