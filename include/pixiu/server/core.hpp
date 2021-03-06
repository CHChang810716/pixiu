#pragma once
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <pixiu/macro.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <stdexcept>
#include <pixiu/error_code_throw.hpp>
#include <pixiu/logger.hpp>
// #include <boost/asio/ssl.hpp>
// #include <future_beast/detect_ssl.hpp>
#include <pixiu/server/session/interface.hpp>
#include <pixiu/server/session/http.hpp>
#include <pixiu/server/session/https.hpp>
#include <boost/coroutine2/all.hpp>
#include <boost/asio/spawn.hpp>
#include "fiber_guard.hpp"
namespace pixiu::server_bits {

template<class IOContextAR, class RequestRouterAR>
struct core
: public std::enable_shared_from_this<
  core<IOContextAR, RequestRouterAR>
>
{
private:
  using error_code        = boost::system::error_code;
  using tcp               = boost::asio::ip::tcp;
  using tcp_endp          = tcp::endpoint;
  using socket_base       = boost::asio::socket_base;
  using tcp_acceptor      = tcp::acceptor;
  using tcp_socket        = tcp::socket;
  using flat_buffer       = boost::beast::flat_buffer;
  using request_router_t  = std::decay_t<RequestRouterAR>;
  using ssl_context       = boost::asio::ssl::context;

  static auto& logger() { return logger::get("core"); }
public:

  // ioc is shared reference, request_router is shared reference
  core(
    IOContextAR&          ioc,
    RequestRouterAR&&     request_router
  )
  : ioc_              (ioc)
  , acceptor_         (ioc)
  , request_router_   (
    std::forward<RequestRouterAR>(request_router)
  )
  , ssl_ctx_          (nullptr)
  {}

  // owning ioc, shared request_router
  core(RequestRouterAR&& request_router)
  : ioc_              ()
  , acceptor_         (ioc_)
  , request_router_   (
    std::forward<RequestRouterAR>(request_router)
  )
  , ssl_ctx_          (nullptr)
  {}


  void listen(tcp_endp ep) {
    boost::asio::spawn(ioc_, [
      _self = this->shared_from_this(), this,
      ep = std::move(ep)
    ](boost::asio::yield_context yield){
      fiber_guard fg;
      error_code ec;

      logger().debug("acceptor open");
      acceptor_.open(ep.protocol(), ec);
      error_code_throw(ec, "acceptor open failed");

      acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
      error_code_throw(ec, "acceptor set_option failed");

      acceptor_.bind(ep, ec);
      error_code_throw(ec, "acceptor bind failed");

      logger().debug("listen");
      acceptor_.listen(
        socket_base::max_listen_connections, ec
      );
      error_code_throw(ec, "acceptor listen failed");
      while(true) {
        tcp_socket                socket      (ioc_);
        logger().debug("async_accept");
        acceptor_.async_accept(socket, yield[ec]);
        if(ec) {
          logger().error("accept failed");
          return ;
        }
        logger().debug("session run");
        spawn_session(
          std::move(socket)
        );
      }
    });
  }
  auto spawn_session(
    tcp_socket&& socket
  ) {
    session_ptr p_session(nullptr);
    if(!ssl_ctx_) {
      p_session.reset(new session::http<request_router_t>(
        acceptor_.get_executor().context(),
        std::move(socket),
        request_router_
      ));
    } else {
      p_session.reset(new session::https<request_router_t> (
        acceptor_.get_executor().context(),
        std::move(socket),
        *ssl_ctx_,
        request_router_
      ));
    }
    return p_session->spawn();
  }
  void listen( const std::string& ip, unsigned short port) {
    listen(tcp_endp{ 
      boost::asio::ip::make_address(ip), port
    });
  }
  void set_tls_context(ssl_context& ctx) {
    ssl_ctx_ = &ctx;
  }
  request_router_t& request_router() {
    return request_router_;
  }
  void run() { ioc_.run(); }

  template<class DU>
  void run_for(DU&& du) { ioc_.run_for(du); }

  ~core() {
    if(acceptor_.is_open()) {
      acceptor_.close();
    }
    logger().debug("core destroy");
  }
private:
  IOContextAR                  ioc_             ;
  tcp_acceptor                 acceptor_        ;
  RequestRouterAR              request_router_  ;
  
  ssl_context*                 ssl_ctx_         ;
};

}