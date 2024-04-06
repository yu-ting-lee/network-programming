#ifndef SOCKET_H
#define SOCKET_H

#include <netinet/in.h>

struct Client {
  Client() {}
  sockaddr_in addr;
  int fd;
};

struct Server {
  Server();

  void start(uint16_t port);
  void bind(sockaddr_in &addr);
  void listen(int nreq);
  Client accept();

  int fd;
};

#endif