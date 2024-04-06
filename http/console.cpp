#include "console.h"

#include <boost/algorithm/string/replace.hpp>

using boost::asio::buffer;
using boost::asio::io_context;
using boost::system::error_code;

void Session::start() {
  auto self(shared_from_this());

  so.async_connect(ep, [this, self](error_code ec) {
    if (!ec) {
      do_read();
    }
  });
}

void Session::do_read() {
  auto self(shared_from_this());

  so.async_read_some(buffer(bytes), [this, self](error_code ec, size_t len) {
    if (!ec) {
      std::string s(bytes.begin(), len);
      html_escape(s);
      std::cout << "<script>document.getElementById('s" << id
                << "').innerHTML += '" << s << "';</script>" << std::endl;
      if (s.find("%") == std::string::npos) {
        do_read();
      } else {
        do_write();
      }
    }
  });
}

void Session::do_write() {
  auto self(shared_from_this());

  if (!lines.empty()) {
    boost::asio::async_write(
        so, buffer(lines[0]), [this, self](error_code ec, size_t len) {
          if (!ec) {
            html_escape(lines[0]);
            std::cout << "<script>document.getElementById('s" << id
                      << "').innerHTML += '<b>" << lines[0] << "</b>';</script>"
                      << std::endl;
            lines.erase(lines.begin());
            do_read();
          }
        });
  }
}

void Session::html_escape(std::string &s) {
  boost::replace_all(s, "&", "&amp;");
  boost::replace_all(s, "\"", "&quot;");
  boost::replace_all(s, "'", "&apos;");
  boost::replace_all(s, "<", "&lt;");
  boost::replace_all(s, ">", "&gt;");
  boost::replace_all(s, "\n", "&NewLine;");
  boost::replace_all(s, "\r", "");
}

int main() {
  std::cout << "Content-type: text/html\r\n\r\n";
  std::cout << R"(
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8" />
        <title>NP Project 3 Sample Console</title>
        <link
        rel="stylesheet"
        href="https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/bootstrap.min.css"
        integrity="sha384-TX8t27EcRE3e/ihU7zmQxVncDAy5uIKz4rEkgIXeMed4M0jlfIDPvg6uqKI2xXr2"
        crossorigin="anonymous"
        />
        <link
        href="https://fonts.googleapis.com/css?family=Source+Code+Pro"
        rel="stylesheet"
        />
        <link
        rel="icon"
        type="image/png"
        href="https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png"
        />
        <style>
        * {
            font-family: 'Source Code Pro', monospace;
            font-size: 1rem !important;
        }
        body {
            background-color: #212529;
        }
        pre {
            color: #cccccc;
        }
        b {
            color: #01b468;
        }
        </style>
    </head>
    <body>
        <table class="table table-dark table-bordered">
        <thead><tr id="h"></tr></thead>
        <tbody><tr id="s"></tr></tbody>
        </table>
    </body>
    </html>)"
            << std::endl;
  std::string req = getenv("QUERY_STRING");
  io_context io;

  if (!req.empty()) {
    args_t args;
    utils::split(args, req, "&");

    for (size_t i = 0; i < args.size(); i += 3) {
      if (args[i].length() <= 3) {
        continue;
      }
      tcp::resolver resolver(io);
      tcp::resolver::query query(args[i].substr(3), args[i + 1].substr(3));
      tcp::resolver::iterator iter = resolver.resolve(query);
      tcp::endpoint ep = *iter;
      tcp::socket so(io);
      std::cout
          << R"(<script>document.getElementById('h').innerHTML += '<th scope="col" id="h)"
          << i / 3 << R"("></th>';</script>)" << std::endl;
      std::cout
          << R"(<script>document.getElementById('s').innerHTML += '<td><pre id="s)"
          << i / 3 << R"(" class="mb-0"></pre></td>';</script>)" << std::endl;

      std::cout << "<script>document.getElementById('h" << i / 3
                << "').innerHTML += '" << args[i].substr(3) << ":" << ep.port()
                << "';</script>" << std::endl;
      std::make_shared<Session>(std::move(so), ep, args[i + 2].substr(3), i / 3)
          ->start();
    }
    io.run();
  }
  return 0;
}