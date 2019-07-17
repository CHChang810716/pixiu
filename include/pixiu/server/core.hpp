#pragma once
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <pixiu/macro.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <stdexcept>
#include <pixiu/error_code_throw.hpp>
#include <pixiu/logger.hpp>
#include <boost/asio/ssl.hpp>
#include <future_beast/detect_ssl.hpp>
#include <pixiu/server/session/interface.hpp>
#include <pixiu/server/session/plain_http.hpp>
namespace pixiu::server {

struct core : public std::enable_shared_from_this<core>
{
private:
  using error_code        = boost::system::error_code;
  using tcp               = boost::asio::ip::tcp;
  using tcp_endp          = tcp::endpoint;
  using io_context        = boost::asio::io_context;
  using socket_base       = boost::asio::socket_base;
  using tcp_acceptor      = tcp::acceptor;
  using tcp_socket        = tcp::socket;
  using flat_buffer       = boost::beast::flat_buffer;
  static auto& logger() { return logger::get("core"); }
public:
  core(
    io_context&           ioc
  )
  : ioc_          (&ioc)
  , acceptor_     (ioc)
  , socket_       (ioc)
  , recv_buffer_  ()
  , req_handler_  (new request_handler())
  {}

  void listen(const tcp_endp& ep) {
    error_code ec;

    acceptor_.open(ep.protocol(), ec);
    error_code_throw(ec, "acceptor open failed");

    acceptor_.bind(ep, ec);
    error_code_throw(ec, "acceptor bind failed");

    acceptor_.listen(
      socket_base::max_listen_connections, ec
    );
    error_code_throw(ec, "acceptor listen failed");
  }
  void listen( const std::string& ip, unsigned short port) {
    listen(tcp_endp{ 
      boost::asio::ip::make_address(ip), port
    });
  }

  void async_accept() {
    auto on_accept = [_self = shared_from_this()](error_code ec) {
      if(ec) {
        logger().error("accept failed");
        return ;
      }
      _self->async_post_session();
      _self->async_accept();
    };
    acceptor_.async_accept(socket_, on_accept);
  }
  ~core() {
    if(acceptor_.is_open()) {
      acceptor_.close();
    }
    logger().debug("core destroy");
  }
private:
  void async_post_session() {
    logger().debug("session run");
    session_ptr p_session = std::make_shared<session::plain_http>(
      *ioc_, 
      std::move(socket_),
      req_handler_,
      std::move(recv_buffer_)
    );
    p_session->async_handle_requests();
  }
  PMEM_GET(io_context*,         ioc           )
  VMEM_GET(tcp_acceptor,        acceptor      )
  VMEM_GET(tcp_socket,          socket        )
  VMEM_GET(flat_buffer,         recv_buffer   )
  VMEM_GET(request_handler_ptr, req_handler   )

};
using core_ptr = std::shared_ptr<core>;

template<class... Args>
auto make_core(Args&&... args) {
  return std::make_shared<core>(std::forward<Args>(args)...);
}

}