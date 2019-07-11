#pragma once
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http.hpp>
#include <httpservpp/config.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/bind_executor.hpp>
#include <httpservpp/logger.hpp>
namespace httpservpp::session {

namespace __http    = boost::beast::http  ;
namespace __beast   = boost::beast        ;
namespace __asio    = boost::asio         ;

struct http_base 
: public std::enable_shared_from_this<http_base>
{
protected:
  using request_body = __http::request<__http::string_body>;
  using strand       = __asio::strand<__asio::io_context::executor_type>;
  using error_code   = boost::system::error_code;
private:
  static auto& logger() { return logger::get("http_base"); }
  template<class Func>
  decltype(auto) make_seq_req_handler(Func&& func) {
    return __asio::bind_executor(
      req_queue_, std::forward<Func>(func)
    );
  }
  template<class Func>
  decltype(auto) make_seq_rep_handler(Func&& func) {
    return __asio::bind_executor(
      rep_queue_, std::forward<Func>(func)
    );
  }
protected:
  void set_request_timeout_handler() {
    // TODO: set handler
    req_timer_.expires_after(config::get().request_timeout());
  }
  
  template<class Http>
  void async_recv_request(Http& derived) {
    using namespace _als;
    auto on_recv_request = [
      _self = shared_from_this(), 
      &derived
    ](error_code ec) {
      // timer close socket
      if(ec == __asio::error::operation_aborted)
        return;
      if(ec == __http::error::end_of_stream)
        return derived.do_eof();
      if(ec)
        return logger().error("receive request failed");
      // TODO: upgrade websocket here

      _self->pending_req_num_ += 1;
      auto req = std::move(_self->req_);
      // TODO: process request -> response
      // TODO: async write response
      _self->async_recv_request(derived);
    };
    if(_self->pending_req_num_ < config::get().max_pending_request_num()) {
      // TODO: read later
    }
    set_request_timeout_handler();
    __http::async_read(
      derived.stream(), recv_buffer_, req_, 
      make_seq_req_handler(on_recv_request)
    )
  }
  template<class Http, bool isRequest, class Body, class Fields>
  void async_send_response(
    Http& derived, 
    __http::message<isRequest, Body, Fields>&& msg
  ) {
    auto on_send_response = [_self = shared_from_this()](error_code ec, bool close) {
      if(ec == __asio::error::operation_aborted)
        return;
      if(ec)
        return logger().error("send response failed");
      if(close) {
        return derived.do_eof();
      }

      _self->pending_req_num_ -= 1;
    };
    __http::async_write(
      derived.stream(), msg,
      make_seq_rep_handler(on_send_response)
    );
  }
protected:
  boost::asio::steady_timer   req_timer_            ;
  boost::beast::flat_buffer   recv_buffer_          ;
  request_body                req_                  ; // workaroud for seperated message
  strand                      req_queue_            ;
  strand                      rep_queue_            ;
  std::atomic_int             pending_req_num_  {0} ;
};

}