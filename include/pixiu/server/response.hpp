#pragma once
#include <boost/beast/http/message.hpp>
#include <variant>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/file_body.hpp>

namespace pixiu::server_bits {

namespace __http    = boost::beast::http  ;
using response_base = std::variant<
  __http::response<__http::string_body>,
  __http::response<__http::empty_body>,
  __http::response<__http::file_body>
>;
struct response : public  response_base{
  using base = response_base;
  using base::base;
  template<class Sender>
  void write(Sender& sender) {
    std::visit([&sender](auto&& arg){
      sender(std::move(arg));
    }, static_cast<base&>(*this));
  }
  template<class Func>
  void apply(Func&& func) {
    std::visit([&func](auto&& arg){
      func(arg);
    }, static_cast<base&>(*this));
  }
};

}