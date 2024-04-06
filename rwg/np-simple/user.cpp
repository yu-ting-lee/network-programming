#include "user.h"

#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>

#include "utils.h"

/* redirect */
void User::set_redirect() {
  if (rfds[0] != -1) {
    utils::dup2(rfds[0], STDOUT_FILENO);
    utils::close(rfds[0]);
  }
}

/* number pipe */
void User::set_npipe_input() {
  if (nfds[0] != nullptr) {
    utils::dup2(nfds[0][0], STDIN_FILENO);
    utils::close_pipe(nfds[0]);
  }
}
void User::set_npipe_output() {
  if (nfds[2] != nullptr) {
    utils::dup2(nfds[2][1], STDERR_FILENO);
  }
  if (nfds[1] != nullptr) {
    utils::dup2(nfds[1][1], STDOUT_FILENO);
    utils::close_pipe(nfds[1]);
  }
}
void User::close_npipe_input() {
  if (nfds[0] != nullptr) {
    utils::close_pipe(nfds[0]);
  }
}
void User::count_npipe() {
  for (auto &n : npipes) {
    n.val--;
  }
}
void User::clear_npipe() {
  auto pred = [this](const NumberPipe &n) { return n.fds == nfds[0]; };
  auto iter = std::find_if(npipes.begin(), npipes.end(), pred);

  if (iter != npipes.end()) {
    delete iter->fds;
    npipes.erase(iter);
  }
}
int *User::search_npipe(int val) {
  auto pred = [val](const NumberPipe &n) { return n.val == val; };
  auto iter = std::find_if(npipes.begin(), npipes.end(), pred);
  return (iter != npipes.end()) ? iter->fds : nullptr;
}

void User::check_redirect(cmds_t &cmds) {
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

void User::check_npipe(cmds_t &cmds) {
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

void User::waitpid() {
  int status;

  while (::waitpid(-1, &status, WNOHANG) > 0) {
    nprocs--;
  }
}

void User::execvp(args_t &args) {
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
