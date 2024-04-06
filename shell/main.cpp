#include <sstream>

#include "shell.h"
#include "utils.h"

int main() {
  setenv("PATH", "bin:.", 1);
  setbuf(stdin, NULL);
  setbuf(stdout, NULL);
  setbuf(stderr, NULL);

  Shell shell;
  std::string line;

  for (;;) {
    std::cout << "% ";
    std::getline(std::cin, line);
    if (std::cin.eof()) {
      break;
    }
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
    shell.count_npipe();
    if (args[0] == "exit") {
      break;
    } else if (args[0] == "printenv") {
      shell.printenv_cmd(args);
    } else if (args[0] == "setenv") {
      shell.setenv_cmd(args);
    } else {
      shell.cmd(args);
    }
    shell.clear_npipe();
  }

  return 0;
}