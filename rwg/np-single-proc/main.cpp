#include <memory.h>
#include <unistd.h>

#include <algorithm>
#include <sstream>

#include "rwg.h"
#include "socket.h"
#include "user.h"
#include "utils.h"

RWG rwg;

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "usage: ./<server> <port>" << std::endl;
    exit(EXIT_FAILURE);
  }
  std::string s = argv[1];
  if (!std::all_of(s.begin(), s.end(), isdigit)) {
    std::cerr << "usage: ./<server> <port>" << std::endl;
    exit(EXIT_FAILURE);
  }
  uint16_t port = std::stoi(argv[1]);
  Server server;
  server.start(port);

  setbuf(stdin, NULL);
  setbuf(stdout, NULL);
  setbuf(stderr, NULL);

  fd_set rfds, afds;
  int nfds = 1024;

  FD_ZERO(&afds);
  FD_SET(server.fd, &afds);

  for (;;) {
    memcpy(&rfds, &afds, sizeof(rfds));
    utils::select(nfds, rfds);

    if (FD_ISSET(server.fd, &rfds)) {
      Client cli = server.accept();
      FD_SET(cli.fd, &afds);
      rwg.login(new User(cli));
    }
    for (int fd = 0; fd < nfds; fd++) {
      if (fd != server.fd && FD_ISSET(fd, &rfds)) {
        User *user = rwg.search_user(fd);
        rwg.backup_env(user);
        rwg.backup_fd();

        utils::dup2(user->fd, STDIN_FILENO);
        utils::dup2(user->fd, STDOUT_FILENO);
        utils::dup2(user->fd, STDERR_FILENO);

        std::string line;
        std::getline(std::cin, line);
        utils::erase_all(line, '\r');
        utils::erase_all(line, '\n');

        args_t args;
        std::istringstream ss(line);
        std::string s;

        while (ss >> s) {
          args.push_back(s);
        }
        if (args.empty()) {
          std::cout << "% ";
          rwg.rollback_fd();
          continue;
        }
        user->count_npipe();
        if (args[0] == "exit") {
          rwg.exit_cmd(user, args, afds);
        } else if (args[0] == "yell") {
          rwg.yell_cmd(user, args, line);
        } else if (args[0] == "tell") {
          rwg.tell_cmd(user, args, line);
        } else if (args[0] == "who") {
          rwg.who_cmd(user, args);
        } else if (args[0] == "name") {
          rwg.name_cmd(user, args);
        } else if (args[0] == "printenv") {
          rwg.printenv_cmd(args);
        } else if (args[0] == "setenv") {
          rwg.setenv_cmd(user, args);
        } else {
          rwg.cmd(user, args, line);
        }
        user->clear_npipe();
        rwg.clear_upipe(user);

        std::cout << "% ";
        rwg.rollback_fd();
        rwg.rollback_env();
      }
    }
  }

  return 0;
}