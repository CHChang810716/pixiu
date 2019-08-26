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
#include <pixiu/server/session/plain_http.hpp>
#include <pixiu/server/request_router.hpp>
#include <boost/coroutine2/all.hpp>
#include <boost/asio/spawn.hpp>
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
  // using io_context        = boost::asio::io_context;
  using socket_base       = boost::asio::socket_base;
  using tcp_acceptor      = tcp::acceptor;
  using tcp_socket        = tcp::socket;
  using flat_buffer       = boost::beast::flat_buffer;
  using request_router_t  = std::decay_t<RequestRouterAR>;
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
  {}

  // owning ioc, shared request_router
  core(RequestRouterAR&& request_router)
  : ioc_              ()
  , acceptor_         (ioc_)
  , request_router_   (
    std::forward<RequestRouterAR>(request_router)
  )
  {}


  void listen(tcp_endp ep) {
    boost::asio::spawn(ioc_, [
      _self = this->shared_from_this(), this,
      ep = std::move(ep)
    ](boost::asio::yield_context yield){
      error_code ec;

      acceptor_.open(ep.protocol(), ec);
      error_code_throw(ec, "acceptor open failed");

      acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
      error_code_throw(ec, "acceptor set_option failed");

      acceptor_.bind(ep, ec);
      error_code_throw(ec, "acceptor bind failed");

      acceptor_.listen(
        socket_base::max_listen_connections, ec
      );
      error_code_throw(ec, "acceptor listen failed");
      for(;;) {
        flat_buffer               recv_buffer ;
        tcp_socket                socket      (ioc_);
        acceptor_.async_accept(socket, yield[ec]);
        if(ec) {
          logger().error("accept failed");
          return ;
        }
        logger().debug("session run");
        session_ptr p_session = std::make_shared<
          session::plain_http<request_router_t>
        >(
          acceptor_.get_executor().context(),
          std::move(socket),
          request_router_,
          std::move(recv_buffer)
        );
        p_session->async_handle_requests();
      }
    });
  }
  void listen( const std::string& ip, unsigned short port) {
    listen(tcp_endp{ 
      boost::asio::ip::make_address(ip), port
    });
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
};
// using core_ptr = std::shared_ptr<core>;
// 
// template<class RequestRouter>
// auto make_core(
//   boost::asio::io_context& ioc,
//   RequestRouter& request_router
// ) {
//   return std::make_shared<
//     core<RequestRouter>
//   >(ioc, request_router));
// }

}