#include "socket.h"

#include <memory.h>
#include <stdio.h>
#include <sys/socket.h>

#include <iostream>

Server::Server() {
  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("error (socket)");
    exit(EXIT_FAILURE);
  }
  int opt = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("error (setsockopt)");
    exit(EXIT_FAILURE);
  }
}
void Server::bind(sockaddr_in &addr) {
  if (::bind(fd, (sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("error (bind)");
    exit(EXIT_FAILURE);
  }
}
void Server::listen(int nreq) {
  if (::listen(fd, nreq) < 0) {
    perror("error (listen)");
    exit(EXIT_FAILURE);
  }
}
Client Server::accept() {
  Client cli;
  socklen_t len = sizeof(cli.addr);

  if ((cli.fd = ::accept(fd, (sockaddr *)&cli.addr, &len)) < 0) {
    perror("error (accept)");
    exit(EXIT_FAILURE);
  };
  return cli;
}

void Server::start(uint16_t port) {
  struct sockaddr_in addr;

  bzero((char *)&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);

  bind(addr);
  listen(5);
}
