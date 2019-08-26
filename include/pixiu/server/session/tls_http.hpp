#pragma once 
#include <boost/asio/ssl/stream.hpp>
#include "http_base_coro.hpp"
#include "interface.hpp"

namespace pixiu::server::session {
namespace __ssl = boost::asio::ssl;
constexpr struct get_ssl_ctx_t {
    auto impl() const { 
        auto ctx = std::make_unique<boost::asio::ssl::context>(
            boost::asio::ssl::context::sslv23
        );
        return ctx;
    }
    auto& operator()() const {
        static auto ctx = impl();
        return *ctx;
    }
} get_ssl_ctx;

template<class RequestRouter>
struct tls_http 
: public http_base<
    tls_http<RequestRouter>, 
    RequestRouter
>
, public interface
{
friend http_base<tls_http<RequestRouter, RequestRouter>>;
private:
  using tcp_socket        = boost::asio::ip::tcp::socket;
  using ssl_stream        = boost::asio::ssl::stream<tcp_socket>;
  using flat_buffer       = boost::beast::flat_buffer;
  using request_router_t  = RequestRouter;
  using base_http_t       = http_base<tls_http<RequestRouter>, RequestRouter>;
  static auto& logger() { return logger::get("tsl_http"); }

public:
  tls_http(
    __asio::io_context&       ioc,
    tcp_socket                socket,
    const request_router_t&   request_router,
    flat_buffer               recv_buffer = flat_buffer()
  )
  : base_http_t         (ioc, request_router)
  , stream_             (std::move(socket), get_ssl_ctx())
  // , recv_buffer_        (std::move(recv_buffer))
  {}

  virtual void async_handle_requests() override {
    boost::asio::spawn(
        base_http_t::read_strand_, 
        [__self = derived(), this](boost::asio::yield_context yield) {
            boost::system::error_code ec;
            stream_.async_handshake(
                __ssl::stream_base::server, 
                yield[ec]
            );
            if(ec)
                return logger().error("handshake failed");
            __self->async_recv_requests_impl(this, yield);
        }
    )
  }

  virtual ~plain_http() override {
    logger().debug("tsl_http destroy");
  }

private:
  template<bool isRequest, class Body, class Fields>
  void async_send_response(
    __http::message<isRequest, Body, Fields> msg
  ) {
    stream_.async_handshake()
    return base_http_t::async_send_response(
      std::move(msg)
    );
  }
  void async_recv_request() {
    return base_http_t::async_recv_request(this->shared_from_this());
  }
  void on_eof() {
    // boost::system::error_code ec;
    // stream_.async_shutdown(ec);
  }
  void on_timeout() {
    // boost::system::error_code ec;
    // socket_.shutdown(__ip::tcp::socket::shutdown_both, ec);
    // socket_.close(ec);
  }
  ssl_stream& stream() {
    return stream_;
  }
  // flat_buffer& recv_buffer() {
  //   return recv_buffer_;
  // }
  ssl_stream          stream_         ;
  // flat_buffer         recv_buffer_    ;
};

}