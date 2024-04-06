#include <sys/wait.h>
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

  setenv("PATH", "bin:.", 1);
  setbuf(stdin, NULL);
  setbuf(stdout, NULL);
  setbuf(stderr, NULL);

  for (;;) {
    Client cli = server.accept();
    pid_t child_pid;
    int status;

    if ((child_pid = fork()) < 0) {
      perror("error (fork)");
      exit(EXIT_FAILURE);
    }
    if (child_pid == 0) {
      utils::dup2(cli.fd, STDIN_FILENO);
      utils::dup2(cli.fd, STDOUT_FILENO);
      utils::dup2(cli.fd, STDERR_FILENO);
      utils::close(cli.fd);

      User *user = new User();
      std::string line;

      for (;;) {
        std::cout << "% ";
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
          continue;
        }
        user->count_npipe();
        if (args[0] == "exit") {
          rwg.exit_cmd(args);
        } else if (args[0] == "printenv") {
          rwg.printenv_cmd(args);
        } else if (args[0] == "setenv") {
          rwg.setenv_cmd(args);
        } else {
          rwg.cmd(user, args);
        }
        user->clear_npipe();
      }
    } else {
      utils::close(cli.fd);
      waitpid(child_pid, &status, 0);
    }
  }

  return 0;
}