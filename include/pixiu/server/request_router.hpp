#pragma once
#include <memory>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include "response.hpp"
#include <regex>
#include "error/all.hpp"
#include <exception>
namespace pixiu::server {

namespace __http    = boost::beast::http  ;
namespace __beast   = boost::beast        ;
namespace __asio    = boost::asio         ;

struct request_router {

  using request               = __http::request<__http::string_body>;

  template<class... Args>
  using handler               = std::function<response(const request&, Args... args)>;

  using head_handler          = std::function<void(response&, const request&)>;
  using get_handler           = handler<>;

  template<class... Args>
  using err_handler          = handler<const error::base&, Args...>;
  template<class... Args>
  using serv_err_handler     = handler<Args...>;

  template<class... Handlers>
  using handler_mapper        = std::map<std::string, std::tuple<Handlers...>>;
  using target_request_mapper = handler_mapper<head_handler, get_handler>;

  static auto& logger() {
    return pixiu::logger::get("request_router");
  }
  static response generic_error_response(const request& req, const error::base& err) {
    return err.create_response(req);
  }
  request_router() {
    on_err_unknown_method_      = generic_error_response;
    on_err_illegal_target_      = generic_error_response;
    on_err_resource_not_found_  = generic_error_response;
    on_err_server_error_        = [](const request& req, std::string what) {
      error::server_error e(what);
      return e.create_response(req);
    };
  }
  void get(const std::string& target_pattern, get_handler&& gh) {
    std::get<1>(on_target_requests_[target_pattern]) = 
      std::move(gh);
  }
  void head(const std::string& target_pattern, head_handler&& hh) {
    std::get<0>(on_target_requests_[target_pattern]) = 
      std::move(hh);
  }
  void on_err_unknown_method(err_handler<>&& eh) {
    on_err_unknown_method_ = eh;
  }
  void on_err_resource_not_found(err_handler<>&& eh) {
    on_err_resource_not_found_ = eh;
  }
  void on_err_server_error(serv_err_handler<std::string>&& eh) {
    on_err_server_error_ = eh;
  }
  void on_err_illegal_target(err_handler<>&& eh) {
    on_err_illegal_target_ = eh;
  }
  auto search_handler(const std::string& target) const {
    for(auto&& [pattern, on_tr] : on_target_requests_) {
      if(std::regex_match(target, std::regex(pattern))) {
        return on_tr;
      }
    }
    throw error::target_not_found(target);
  }
  template<class Func>
  void operator()(
    request  req, 
    Func&&   send
  ) const {
    try {
      // Request path must be absolute and not contain "..".
      if( req.target().empty() ||
          req.target()[0] != '/' ||
          req.target().find("..") != boost::beast::string_view::npos
      ) throw error::illegal_target(req.target());

      auto handlers = search_hander(req.target());

      // Respond to HEAD request
      switch(req.method()) {
        case __http::verb::head: 
          return method_case_head(
            std::move(req), 
            std::move(send),
            handlers
          );
        case __http::verb::get:
          return method_case_get(
            std::move(req), 
            std::move(send),
            handlers
          );
        default:
          throw error::unknown_method(req.method())
      };
    } catch (const error::illegal_target& err) {
      on_err_illegal_target(req, err);
    } catch (const error::target_not_found& err) {
      on_err_target_not_found(req, err);
    } catch (const error::unknown_method& err) {
      on_err_unknown_method(req, err);
    } catch (const std::exception& e) {
      on_err_server_error(req, e.what());
    }
  }
private:
  void generic_header_config(response& rep, const request& req) {
    std::visit(rep, [&rep](auto&& inter_rep){
      if(inter_rep.result() == __http::status::unknown) {
        inter_rep.result(__http::status::ok);
      }
      if(inter_rep.version().empty()) {
        inter_rep.version(rep.version());
      }
      if(inter_rep.at(__http::field::server).empty()) {
        inter_rep.set(__http::field::server, BOOST_BEAST_VERSION_STRING);
      }
      inter_rep.keep_alive(req.keep_alive());
    });
  }
  template<class Func, class HandlerTuple>
  void method_case_head(request req, Func&& send, HandlerTuple& handlers) {
    logger().debug("head request");
    auto [head_h, get_h] = handlers;
    response rep(__http::response<__http::empty_body>());
    generic_header_config(res, req);
    head_h(rep, std::move(req));
    return rep.write(send);
  }
  template<class Func, class HandlerTuple>
  void method_case_get(request req, Func&& send, HandlerTuple& handlers) {
    logger().debug("get request");
    auto [head_h, get_h] = handlers;
    auto rep = get_h(req);
    generic_header_config(res, req);
    if(head_h) {
      head_h(rep, std::move(req));
    } else {
      std::visit(rep, [](auto&& inter_rep){
        if(inter_rep.payload_size() == 0) {
          inter_rep.prepare_payload();
        }
        if(inter_rep.get(__http::field::content_type).empty()) {
          inter_rep.set(__http::field::content_type, "text/html");
        }
      });
    }
    return rep.write(send);
  }
  err_handler<>                 on_err_unknown_method_      ;
  err_handler<>                 on_err_illegal_target_      ;
  err_handler<>                 on_err_resource_not_found_  ;
  serv_err_handler<std::string> on_err_server_error_        ;
  target_request_mapper         on_target_requests_         ;

};

}