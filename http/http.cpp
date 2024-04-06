#include "http.h"

#include <unistd.h>

using boost::asio::buffer;
using boost::system::error_code;

void Session::do_read() {
  auto self(shared_from_this());

  so.async_read_some(buffer(bytes), [this, self](error_code ec, size_t len) {
    if (!ec) {
      std::string s(bytes.begin(), len);
      utils::split(lines, s, "\r\n");
      do_write();
    }
  });
}

void Session::do_write() {
  auto self(shared_from_this());

  std::string s = "HTTP/1.1 200 OK\r\n";
  boost::asio::async_write(
      so, buffer(s), [this, self](error_code ec, size_t len) {
        if (!ec) {
          args_t args;
          switch (fork()) {
            case -1: {
              perror("error (fork)");
              exit(EXIT_FAILURE);
            }
            case 0: {
              utils::split(args, lines[0], " ");
              utils::setenv("REQUEST_METHOD", args[0]);
              utils::setenv("REQUEST_URI", args[1]);
              utils::setenv("SERVER_PROTOCOL", args[2]);
              std::string line = args[1];

              utils::split(args, lines[1], " ");
              utils::setenv("HTTP_HOST", args[1]);

              utils::split(args, line, "?");
              utils::setenv("QUERY_STRING", (args.size() > 1) ? args[1] : "");
              std::string cgi = args[0].substr(1);

              utils::setenv("SERVER_ADDR", loc.address().to_string());
              utils::setenv("SERVER_PORT", std::to_string(loc.port()));
              utils::setenv("REMOTE_ADDR", rem.address().to_string());
              utils::setenv("REMOTE_PORT", std::to_string(rem.port()));

              utils::dup2(so.native_handle(), STDIN_FILENO);
              utils::dup2(so.native_handle(), STDOUT_FILENO);
              utils::execvp(cgi);
            }
            default: {
              so.close();
            }
          }
        }
      });
}

void Server::do_accept() {
  ac.async_accept([this](error_code ec, tcp::socket so) {
    if (!ec) {
      std::make_shared<Session>(std::move(so))->start();
    }
    do_accept();
  });
}

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
  io_context io;
  Server server(io, port);

  utils::setenv("PATH", "/usr/bin:cgi:.");
  io.run();

  return 0;
}