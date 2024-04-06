#include "shell.h"

#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>

#include "utils.h"

/* redirect */
void Shell::set_redirect() {
  if (rfds[0] != -1) {
    utils::dup2(rfds[0], STDOUT_FILENO);
    utils::close(rfds[0]);
  }
}

/* number pipe */
void Shell::set_npipe_input() {
  if (nfds[0] != nullptr) {
    utils::dup2(nfds[0][0], STDIN_FILENO);
    utils::close_pipe(nfds[0]);
  }
}
void Shell::set_npipe_output() {
  if (nfds[2] != nullptr) {
    utils::dup2(nfds[2][1], STDERR_FILENO);
  }
  if (nfds[1] != nullptr) {
    utils::dup2(nfds[1][1], STDOUT_FILENO);
    utils::close_pipe(nfds[1]);
  }
}
void Shell::close_npipe_input() {
  if (nfds[0] != nullptr) {
    utils::close_pipe(nfds[0]);
  }
}
void Shell::count_npipe() {
  for (auto &n : npipes) {
    n.val--;
  }
}
void Shell::clear_npipe() {
  auto pred = [this](const NumberPipe &n) { return n.fds == nfds[0]; };
  auto iter = std::find_if(npipes.begin(), npipes.end(), pred);

  if (iter != npipes.end()) {
    delete iter->fds;
    npipes.erase(iter);
  }
}
int *Shell::search_npipe(int val) {
  auto pred = [val](const NumberPipe &n) { return n.val == val; };
  auto iter = std::find_if(npipes.begin(), npipes.end(), pred);
  return (iter != npipes.end()) ? iter->fds : nullptr;
}

void Shell::check_redirect(cmds_t &cmds) {
  if (cmds.empty() || cmds.back().size() < 3) {
    return;
  }
  args_t &args = cmds.back();
  if (*(args.end() - 2) != ">") {
    return;
  }
  const char *file = args.back().c_str();
  rfds[0] = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0640);
  args.erase(args.end() - 2, args.end());
}

void Shell::check_npipe(cmds_t &cmds) {
  nfds[0] = search_npipe(0);
  if (cmds.empty() || cmds.back().size() < 2) {
    return;
  }
  args_t &args = cmds.back();
  if (args.back().length() < 2 ||
      (args.back()[0] != '|' && args.back()[0] != '!')) {
    return;
  }
  std::string r = args.back().substr(1);
  if (!std::all_of(r.begin(), r.end(), isdigit)) {
    std::cerr << "usage: <command> |<line-count>" << std::endl;
    cmds.clear();
    return;
  }
  int val = std::stoi(r);
  if ((nfds[1] = search_npipe(val)) == nullptr) {
    int *fds = new int[2];
    utils::pipe(fds);
    npipes.emplace_back(val, fds);
    nfds[1] = fds;
  }
  nfds[2] = (args.back()[0] == '!') ? nfds[1] : nullptr;
  args.pop_back();
}

void Shell::waitpid() {
  int status;

  while (::waitpid(-1, &status, WNOHANG) > 0) {
    nprocs--;
  }
}

void Shell::execvp(args_t &args) {
  pid_t child_pid;

  if (nfds[1] == nullptr) {
    utils::execvp(utils::to_argv(args));
  }
  while ((child_pid = fork()) < 0) {
    usleep(1000);
  }
  if (child_pid == 0) {
    utils::execvp(utils::to_argv(args));
  }
  exit(EXIT_SUCCESS);
}

void Shell::printenv_cmd(args_t &args) {
  if (args.size() != 2) {
    std::cerr << "usage: printenv <var>" << std::endl;
    return;
  }
  char *env = getenv(args[1].c_str());
  if (env != nullptr) {
    std::cout << env << std::endl;
  }
}

void Shell::setenv_cmd(args_t &args) {
  if (args.size() != 3) {
    std::cerr << "usage: setenv <var> <value>" << std::endl;
    return;
  }
  if (setenv(args[1].c_str(), args[2].c_str(), 1) < 0) {
    perror("error (setenv)");
    exit(EXIT_FAILURE);
  }
}

void Shell::cmd(args_t &args) {
  rfds.fill(-1);
  nfds.fill(nullptr);

  cmds_t cmds;
  auto iter = std::find(args.begin(), args.end(), "|");

  while (iter != args.end()) {
    cmds.push_back(args_t(args.begin(), iter));
    args.erase(args.begin(), ++iter);
    iter = std::find(args.begin(), args.end(), "|");
  }
  cmds.push_back(args);

  check_redirect(cmds);
  check_npipe(cmds);

  nprocs = cmds.size();
  int fds[cmds.size() - 1][2];
  pid_t child_pid;

  for (size_t i = 0; i < cmds.size(); i++) {
    if (i != cmds.size() - 1) {
      utils::pipe(fds[i]);
    }
    while ((child_pid = fork()) < 0) {
      waitpid();
    }
    if (child_pid == 0) { /* child process */
      if (i != 0) {
        utils::dup2(fds[i - 1][0], STDIN_FILENO);
        utils::close_pipe(fds[i - 1]);
      } else {
        set_npipe_input();
      }
      if (i != cmds.size() - 1) {
        utils::dup2(fds[i][1], STDOUT_FILENO);
        utils::close_pipe(fds[i]);
      } else {
        set_npipe_output();
        set_redirect();
      }
      execvp(cmds[i]);

    } else { /* parent process */
      if (i != 0) {
        utils::close_pipe(fds[i - 1]);
      } else {
        close_npipe_input();
      }
    }
  }
  while (nprocs > 0) {
    waitpid();
  }
}
