#pragma once
#include <boost/beast/http/message.hpp>
#include <variant>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/file_body.hpp>

namespace pixiu::server {

namespace __http    = boost::beast::http  ;

struct response : public std::variant<
  __http::response<__http::string_body>,
  __http::response<__http::empty_body>,
  __http::response<__http::file_body>
> {
  template<class Sender>
  void write(Sender& sender) {
    std::visit([&sender](auto&& arg){
      sender(std::move(arg));
    }, *this);
  }
};

}