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
  using request_body = __http::request<__http::string_body>;
  // using sender = std::function<void()
  static auto& logger() {
    return pixiu::logger::get("request_handler");
  }
  template<class Func>
  void operator()(
    request_body  req, 
    Func&&        send
  ) const {
    // Returns a bad request response
    auto const bad_request =
    [&req](boost::beast::string_view why) {
      __http::response<__http::string_body> res{
        __http::status::bad_request, req.version()
      };
      res.set(__http::field::server, BOOST_BEAST_VERSION_STRING);
      res.set(__http::field::content_type, "text/html");
      res.keep_alive(req.keep_alive());
      res.body() = why.to_string();
      res.prepare_payload();
      return res;
    };

    // Returns a not found response
    auto const not_found =
    [&req](boost::beast::string_view target) {
        __http::response<__http::string_body> res{
          __http::status::not_found, req.version()
        };
        res.set(__http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(__http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "The resource '" + target.to_string() + "' was not found.";
        res.prepare_payload();
        return res;
    };

    // Returns a server error response
    auto const server_error =
    [&req](boost::beast::string_view what) {
        __http::response<__http::string_body> res{
          __http::status::internal_server_error, req.version()
        };
        res.set(__http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(__http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "An error occurred: '" + what.to_string() + "'";
        res.prepare_payload();
        return res;
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

    // // Build the path to the requested file
    // std::string path = path_cat(doc_root, req.target());
    // if(req.target().back() == '/')
    //     path.append("index.html");

    // // Attempt to open the file
    // boost::beast::error_code ec;
    // http::file_body::value_type body;
    // body.open(path.c_str(), boost::beast::file_mode::scan, ec);

    // // Handle the case where the file doesn't exist
    // if(ec == boost::system::errc::no_such_file_or_directory)
    //     return send(not_found(req.target()));

    // // Handle an unknown error
    // if(ec)
    //     return send(server_error(ec.message()));
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

};

}