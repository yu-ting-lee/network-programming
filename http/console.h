#ifndef CONSOLE_H
#define CONSOLE_H

#include <array>
#include <boost/asio.hpp>
#include <fstream>

#include "utils.h"

using boost::asio::ip::tcp;

constexpr int MAX_MSGLEN = 1024;

class Session : public std::enable_shared_from_this<Session> {
 public:
  Session(tcp::socket so, tcp::endpoint ep, std::string file, int id)
      : ep(ep), so(std::move(so)), id(id) {
    std::string s;
    std::ifstream fs("test_case/" + file);

    while (std::getline(fs, s)) {
      lines.push_back(s + "\n");
    }
    fs.close();
  };
  void start();

 private:
  void html_escape(std::string &s);
  void do_read();
  void do_write();

  args_t lines;
  std::array<char, MAX_MSGLEN> bytes;
  tcp::endpoint ep;
  tcp::socket so;
  int id;
};

#endif