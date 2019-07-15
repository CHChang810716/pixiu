#pragma once
#include "http_base.hpp"
#include <boost/asio/ip/tcp.hpp>
namespace httpservpp::session {

struct plain_http 
: public http_base 
, public std::enable_shared_from_this<plain_http> 
{
private:
  using tcp_socket        = boost::asio::ip::tcp::socket;
  using flat_buffer       = boost::beast::flat_buffer;
  static auto& logger() { return logger::get("plain_http"); }
public:
  plain_http(
    __asio::io_context&     ioc,
    tcp_socket              socket,
    request_handler_ptr     req_handler,
    flat_buffer             recv_buffer = flat_buffer()
  )
  : http_base         (ioc, req_handler)
  , socket_           (std::move(socket))
  , recv_buffer_      (std::move(recv_buffer))
  {}

  void async_recv_request() {
    return http_base::async_recv_request(shared_from_this());
  }

  template<class Http, bool isRequest, class Body, class Fields>
  void async_send_response(
    __http::message<isRequest, Body, Fields> msg
  ) {
    return http_base::async_send_response(std::move(msg));
  }
  tcp_socket& stream() {
    return socket_;
  }
  flat_buffer& recv_buffer() {
    return recv_buffer_;
  }
private:
  tcp_socket          socket_         ;
  flat_buffer         recv_buffer_    ;
};
using plain_http_ptr = std::shared_ptr<plain_http>;
}