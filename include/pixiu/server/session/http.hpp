#pragma once
#include "http_base.hpp"
#include <boost/asio/ip/tcp.hpp>
#include "interface.hpp"
#include <pixiu/server/fiber_guard.hpp>
namespace pixiu::server_bits::session {
namespace __ip = boost::asio::ip;
template<class RequestRouter>
struct http 
: public http_base<
  http<RequestRouter>, 
  RequestRouter
> 
, public std::enable_shared_from_this<
  http<RequestRouter>
> 
, public interface
{
friend http_base<http<RequestRouter>, RequestRouter>;
private:
  using tcp_socket        = boost::asio::ip::tcp::socket;
  using flat_buffer       = boost::beast::flat_buffer;
  using request_router_t  = RequestRouter;
  using base_http_t       = http_base<http<RequestRouter>, RequestRouter>;
  static auto& logger() { return logger::get("http"); }

public:
  http(
    __asio::io_context&       ioc,
    tcp_socket                socket,
    const request_router_t&   request_router
  )
  : base_http_t         (ioc, request_router)
  , socket_             (std::move(socket))
  {}

  virtual void spawn() override {
    boost::asio::spawn(
      base_http_t::runner_strand_, 
      [__self = this->derived(), this](boost::asio::yield_context yield) {
        fiber_guard fg;
        __self->async_run_impl(yield);
      }
    );
  }

  virtual bool is_closed() const override {
    return !socket_.is_open();
  }

  virtual ~http() override {
    logger().debug("http destroy");
  }

private:
  template<bool isRequest, class Body, class Fields>
  void async_send_response(
    __http::message<isRequest, Body, Fields> msg,
    boost::asio::yield_context yield
  ) {
    return base_http_t::async_send_response_impl(
      std::move(msg), yield
    );
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