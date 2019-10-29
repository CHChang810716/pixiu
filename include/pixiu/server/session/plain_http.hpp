#pragma once
#include "http_base_coro.hpp"
#include <boost/asio/ip/tcp.hpp>
#include "interface.hpp"
namespace pixiu::server_bits::session {
namespace __ip = boost::asio::ip;
template<class RequestRouter>
struct plain_http 
: public http_base<
  plain_http<RequestRouter>, 
  RequestRouter
> 
, public std::enable_shared_from_this<
  plain_http<RequestRouter>
> 
, public interface
{
friend http_base<plain_http<RequestRouter>, RequestRouter>;
private:
  using tcp_socket        = boost::asio::ip::tcp::socket;
  using flat_buffer       = boost::beast::flat_buffer;
  using request_router_t  = RequestRouter;
  using base_http_t       = http_base<plain_http<RequestRouter>, RequestRouter>;
  static auto& logger() { return logger::get("plain_http"); }

public:
  plain_http(
    __asio::io_context&       ioc,
    tcp_socket                socket,
    const request_router_t&   request_router,
    flat_buffer               recv_buffer
  )
  : base_http_t         (ioc, request_router)
  , socket_             (std::move(socket))
  , recv_buffer_        (std::move(recv_buffer))
  {}

  virtual void async_handle_requests() override {
    base_http_t::async_recv_requests();
  }

  virtual ~plain_http() override {
    logger().debug("plain_http destroy");
  }

private:
  template<bool isRequest, class Body, class Fields>
  void async_send_response(
    __http::message<isRequest, Body, Fields> msg
  ) {
    return base_http_t::async_send_response(
      std::move(msg)
    );
  }
  void async_recv_request() {
    return base_http_t::async_recv_request(this->shared_from_this());
  }
  void on_eof(boost::asio::yield_context& yield) {
    boost::system::error_code ec;
    socket_.shutdown(__ip::tcp::socket::shutdown_send, ec);
  }
  void on_timeout() {
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
}