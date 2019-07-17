#pragma once
#include "http_base.hpp"
#include <boost/asio/ip/tcp.hpp>
#include "interface.hpp"
namespace httpservpp::server::session {
namespace __ip = boost::asio::ip;
struct plain_http 
: public http_base 
, public std::enable_shared_from_this<plain_http> 
, public interface
{
friend http_base;
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

  virtual void async_handle_requests() override {
    this->async_recv_request();
  }

  virtual ~plain_http() override {
    logger().debug("plain_http destroy");
  }

private:
  template<bool isRequest, class Body, class Fields>
  void async_send_response(
    __http::message<isRequest, Body, Fields> msg
  ) {
    return http_base::async_send_response(
      shared_from_this(), std::move(msg)
    );
  }
  void async_recv_request() {
    return http_base::async_recv_request(shared_from_this());
  }
  void do_eof() {
    error_code ec;
    socket_.shutdown(__ip::tcp::socket::shutdown_send, ec);
  }
  void do_request_timeout() {
    boost::system::error_code ec;
    socket_.shutdown(__ip::tcp::socket::shutdown_both, ec);
    socket_.close(ec);
  }
  tcp_socket& stream() {
    return socket_;
  }
  flat_buffer& recv_buffer() {
    return recv_buffer_;
  }
  tcp_socket          socket_         ;
  flat_buffer         recv_buffer_    ;
};
using plain_http_ptr = std::shared_ptr<plain_http>;
}