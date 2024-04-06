#ifndef HTTP_H
#define HTTP_H

#include <array>
#include <boost/asio.hpp>

#include "utils.h"

using boost::asio::io_context;
using boost::asio::ip::tcp;

constexpr int MAX_MSGLEN = 1024;

class Session : public std::enable_shared_from_this<Session> {
 public:
  Session(tcp::socket so)
      : loc(so.local_endpoint()),
        rem(so.remote_endpoint()),
        so(std::move(so)) {}

  void start() { do_read(); }

 private:
  void do_read();
  void do_write();

  args_t lines;
  std::array<char, MAX_MSGLEN> bytes;
  tcp::endpoint loc, rem;
  tcp::socket so;
};

class Server {
 public:
  Server(io_context &io, uint16_t p) : ac(io, tcp::endpoint(tcp::v4(), p)) {
    do_accept();
  }

 private:
  void do_accept();

  tcp::acceptor ac;
};

#endif