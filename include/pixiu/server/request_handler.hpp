#pragma once
#include <memory>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/http/string_body.hpp>
namespace pixiu::server {

namespace __http    = boost::beast::http  ;
namespace __beast   = boost::beast        ;
namespace __asio    = boost::asio         ;

struct request_handler {
  using request_body        = __http::request<__http::string_body>;
  using response            = __http::response<__http::string_body>;
  using empty_response      = __http::response<__http::empty_body>;
  using handler             = std::function<response(request_body)>;
  using server_err_handler  = std::function<response(request_body, std::string)>;
  using rep_nobody_handler  = std::function<empty_response(request_body)>;
  template<class Handler>
  using request_router      = std::map<std::string, Handler>;
  static auto& logger() {
    return pixiu::logger::get("request_handler");
  }
  request_handler() {
    on_err_unknown_method_ = [this](request_body req) -> response {
      return bad_request(req, "Unknown HTTP method");
    };
    on_err_illegal_target_ = [this](request_body req) -> response {
      return bad_request(req, "Illegal request target");
    };
    on_err_resource_not_found_ = [this](request_body req) -> response {
      return not_found(req);
    };
    on_err_server_error_ = [this](request_body req, std::string what) -> response {
      return server_error(req, what);
    };
  }
  template<class Func>
  void operator()(
    request_body  req, 
    Func&&        send
  ) const {

    // Returns a server error response
    auto const server_error =
    [&req](boost::beast::string_view what) {
    };

    // Make sure we can handle the method
    if( req.method() != __http::verb::get &&
        req.method() != __http::verb::head)
        return send(bad_request("Unknown HTTP-method"));

    // Request path must be absolute and not contain "..".
    if( req.target().empty() ||
        req.target()[0] != '/' ||
        req.target().find("..") != boost::beast::string_view::npos)
        return send(bad_request("Illegal request-target"));

    __http::string_body::value_type body;
    body = "server works !!!";


    // Respond to HEAD request
    if(req.method() == __http::verb::head)
    {
      logger().debug("head request");
      __http::response<__http::empty_body> res{__http::status::ok, req.version()};
      res.set(__http::field::server, BOOST_BEAST_VERSION_STRING);
      res.set(__http::field::content_type, "text/plain");
      res.content_length(body.size());
      res.keep_alive(req.keep_alive());
      return send(std::move(res));
    }

    // Respond to GET request
    auto body_size = body.size();
    logger().debug("get request");
    __http::response<__http::string_body> res{
      std::piecewise_construct,
      std::make_tuple(std::move(body)),
      std::make_tuple(__http::status::ok, req.version())
    };
    res.set(__http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(__http::field::content_type, "text/plain");
    res.content_length(body_size);
    res.keep_alive(req.keep_alive());
    return send(std::move(res));
  }
private:
  response bad_request(request_body req, std::string why) {
    __http::response<__http::string_body> res{
      __http::status::bad_request, req.version()
    };
    res.set(__http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(__http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = why;
    res.prepare_payload();
    return res;
  }
  response not_found(request_body req) {
    __http::response<__http::string_body> res{
      __http::status::not_found, req.version()
    };
    res.set(__http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(__http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = "The resource '" + req.target().to_string() + "' was not found.";
    res.prepare_payload();
    return res;
  }
  response server_error(request_body req, std::string what) {
    __http::response<__http::string_body> res{
      __http::status::internal_server_error, req.version()
    };
    res.set(__http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(__http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = "An error occurred: '" + what + "'";
    res.prepare_payload();
    return res;

  }
  handler               on_err_unknown_method_      ;
  handler               on_err_illegal_target_      ;
  handler               on_err_resource_not_found_  ;
  server_err_handler    on_err_server_error_        ;
  request_router<
    rep_nobody_handler
  >                     on_head_request_        ;
  request_router<
    handler
  >                     on_get_request_         ;

};

}