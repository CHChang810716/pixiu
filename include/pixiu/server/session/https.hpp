#pragma once 
#include <boost/asio/ssl/stream.hpp>
#include "interface.hpp"
#include "http_base.hpp"
#include <pixiu/server/fiber_guard.hpp>

namespace pixiu::server_bits::session {
namespace __ssl = boost::asio::ssl;

template<class RequestRouter>
struct https 
: public http_base<
  https<RequestRouter>, 
  RequestRouter
>
, public std::enable_shared_from_this<
  https<RequestRouter>
>
, public interface
{
friend http_base<https<RequestRouter>, RequestRouter>;
private:
  using tcp_socket        = boost::asio::ip::tcp::socket;
  using ssl_stream        = boost::asio::ssl::stream<tcp_socket>;
  using flat_buffer       = boost::beast::flat_buffer;
  using request_router_t  = RequestRouter;
  using base_http_t       = http_base<https<RequestRouter>, RequestRouter>;

  static auto& logger() { return logger::get("https"); }

public:
  https(
    __asio::io_context&       ioc,
    tcp_socket                socket,
    __ssl::context&           ssl_ctx,
    const request_router_t&   request_router
  )
  : base_http_t         (ioc, request_router)
  , stream_             (std::move(socket), ssl_ctx)
  {}

  virtual void spawn() override {
    boost::asio::spawn(
      base_http_t::runner_strand_, 
      [__self = this->derived(), this](boost::asio::yield_context yield) {
        fiber_guard fg;
        boost::system::error_code ec;
        // TODO: timeout detection
        // base_http_t::set_timer();
        logger().debug("handshake");
        stream_.async_handshake(
          __ssl::stream_base::server, 
          yield[ec]
        );
        if(ec)
          return logger().error("handshake failed, {}", ec.message());
        __self->async_run_impl(yield);
      }
    );
  }

  virtual bool is_closed() const override {
    return !stream_.lowest_layer().is_open();
  }

  virtual ~https() override {
    logger().debug("tsl_http destroy");
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
    logger().debug("async_shutdown");
    stream_.async_shutdown(yield[ec]);
    if(ec) {
      return logger().debug("shutdown failed, {}", ec.message());
    }
    stream_.lowest_layer().close(ec);
    if(ec) {
      return logger().debug("socket close failed, {}", ec.message());
    }
  }
  void on_timeout() {
    logger().debug("shutdown");
    stream_.async_shutdown([__self = base_http_t::derived()](auto ec){
      if(ec) {
        return logger().debug("shutdown failed, {}", ec.message());
      }
      __self->stream().lowest_layer().close(ec);
      if(ec) {
        return logger().debug("socket close failed, {}", ec.message());
      }
    });
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