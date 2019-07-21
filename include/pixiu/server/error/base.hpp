#pragma once
#include <stdexcept>
#include <pixiu/server/response.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
namespace pixiu::server::error {

namespace __http    = boost::beast::http  ;
namespace __beast   = boost::beast        ;

struct base : public std::runtime_error 
{
  using request = __http::request<__http::string_body>;
  using __base = std::runtime_error;
  using __base::__base;
  virtual response create_response(
    const request&       req
  ) const noexcept = 0;
protected:
  response basic_error_response(
    __http::status        error_status,
    const request&        req
  ) const noexcept {
    __http::response<__http::string_body> res{
      error_status, req.version()
    };
    res.set(__http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(__http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = this->what();
    res.prepare_payload();
    return response(res);
  }

};
}