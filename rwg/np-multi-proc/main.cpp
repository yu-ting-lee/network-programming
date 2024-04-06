#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <sstream>

#include "rwg.h"
#include "socket.h"
#include "user.h"
#include "utils.h"

void sigint_hdl(int signo);
void sigusr1_hdl(int signo);
void sigchld_hdl(int signo);

pid_t pid = getpid();
RWG rwg;

int main(int argc, char **argv) {
  utils::sigaction(SIGINT, sigint_hdl);
  utils::sigaction(SIGUSR1, sigusr1_hdl);
  utils::sigaction(SIGCHLD, sigchld_hdl);

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

  rwg.shm_attach();

  for (int i = 0; i < MAX_CLIENT; i++) {
    rwg.stat[i] = false;
  }
  for (;;) {
    Client cli = server.accept();
    pid_t child_pid;
    rwg.nprocs++;

    while ((child_pid = fork()) < 0) {
      rwg.waitpid();
    }
    if (child_pid == 0) {
      utils::dup2(cli.fd, STDIN_FILENO);
      utils::dup2(cli.fd, STDOUT_FILENO);
      utils::dup2(cli.fd, STDERR_FILENO);
      utils::close(cli.fd);

      User *user = new User();
      Info info(cli);
      rwg.login(info);

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
        } else if (args[0] == "yell") {
          rwg.yell_cmd(args, line);
        } else if (args[0] == "tell") {
          rwg.tell_cmd(args, line);
        } else if (args[0] == "who") {
          rwg.who_cmd(args);
        } else if (args[0] == "name") {
          rwg.name_cmd(args);
        } else if (args[0] == "printenv") {
          rwg.printenv_cmd(args);
        } else if (args[0] == "setenv") {
          rwg.setenv_cmd(args);
        } else {
          rwg.cmd(user, args, line);
        }
        user->clear_npipe();
      }
    } else {
      utils::close(cli.fd);
    }
  }

  return 0;
}

void sigint_hdl(int signo) {
  if (getpid() == pid) {
    while (rwg.nprocs > 0) {
      rwg.waitpid();
    }
    rwg.shm_detach();
    std::cout << "caught SIGINT" << std::endl;
  }
  exit(EXIT_SUCCESS);
}

void sigusr1_hdl(int signo) {
  if (rwg.msg->type == READ) {
    std::cout << rwg.msg->msg;
  }
  if (rwg.msg->type == OPEN) {
    int send = std::stoi(rwg.msg->msg) - 1;
    std::string file = rwg.get_file(send, rwg.id - 1);
    rwg.upipes[send] = new UserPipe(file);
    utils::open(&rwg.upipes[send]->fd, file.c_str(), O_RDONLY);
  }
  utils::sem_post(rwg.lock);
}

void sigchld_hdl(int signo) {
  if (getpid() == pid) {
    rwg.waitpid();
  }
}
