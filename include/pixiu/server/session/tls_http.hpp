#pragma once 
#include <boost/asio/ssl/stream.hpp>
#include "interface.hpp"
#include "http_base_coro.hpp"

namespace pixiu::server_bits::session {
namespace __ssl = boost::asio::ssl;

template<class RequestRouter>
struct tls_http 
: public http_base<
  tls_http<RequestRouter>, 
  RequestRouter
>
, public std::enable_shared_from_this<
  tls_http<RequestRouter>
>
, public interface
{
friend http_base<tls_http<RequestRouter>, RequestRouter>;
private:
  using tcp_socket        = boost::asio::ip::tcp::socket;
  using ssl_stream        = boost::asio::ssl::stream<tcp_socket>;
  using flat_buffer       = boost::beast::flat_buffer;
  using request_router_t  = RequestRouter;
  using base_http_t       = http_base<tls_http<RequestRouter>, RequestRouter>;

  static auto& logger() { return logger::get("tls_http"); }

public:
  tls_http(
    __asio::io_context&       ioc,
    tcp_socket                socket,
    __ssl::context&           ssl_ctx,
    const request_router_t&   request_router,
    flat_buffer               recv_buffer
  )
  : base_http_t         (ioc, request_router)
  , stream_             (std::move(socket), ssl_ctx)
  , recv_buffer_        (std::move(recv_buffer))
  {}

  virtual void async_handle_requests() override {
    boost::asio::spawn(
        base_http_t::read_strand_, 
        [__self = this->derived(), this](boost::asio::yield_context yield) {
            boost::system::error_code ec;
            // TODO: timeout detection
            base_http_t::set_timer();
            stream_.async_handshake(
                __ssl::stream_base::server, 
                yield[ec]
            );
            if(ec)
                return logger().error("handshake failed");
            __self->async_recv_requests_impl(yield);
        }
    );
  }

  virtual ~tls_http() override {
    logger().debug("tsl_http destroy");
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
    stream_.async_shutdown(yield[ec]);
    if(ec) {
      return logger().debug("shutdown failed");
    }
  }
  void on_timeout() {
    boost::system::error_code ec;
    stream_.shutdown(ec);
  }
  ssl_stream& stream() {
    return stream_;
  }
  flat_buffer& recv_buffer() {
    return recv_buffer_;
  }
  ssl_stream          stream_         ;
  flat_buffer         recv_buffer_    ;
};

}