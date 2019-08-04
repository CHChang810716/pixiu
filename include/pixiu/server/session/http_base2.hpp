#pragma once
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http.hpp>
#include <pixiu/config.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/bind_executor.hpp>
#include <pixiu/logger.hpp>
#include <pixiu/server/request_router.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/lockfree/spsc_queue.hpp>

namespace pixiu::server::session {

namespace __http    = boost::beast::http  ;
namespace __beast   = boost::beast        ;
namespace __asio    = boost::asio         ;

template<class RequestRouter>
struct http_base 
{
private:
  using tcp_socket        = __asio::ip::tcp::socket;
  using flat_buffer       = boost::beast::flat_buffer;
  using request_router_t  = RequestRouter;
  static auto& logger() { return logger::get("http_base"); }
public:
  http_base(
    __asio::io_context&      ioc, 
    const request_router_t&  request_router
  ) 
  : request_router_     (&request_router)
  , request_timeout_    (15)
  , max_pending_req_num_(16)
  {}
  void request_timeout(int sec) {
    request_timeout_ = sec;
  }
  void max_pending_request_num(int num) {
    max_pending_req_num_ = num;
  }
  ~http_base() {
    logger().debug("http_base destroy");
  }
protected:
  using strand       = __asio::strand<__asio::io_context::executor_type>;
  using error_code   = boost::system::error_code;

  template<class AsyncStream>
  void async_handle_requests(AsyncStream stream) {
    using request      = __http::request<__http::string_body>;
    boost::lockfree::spsc_queue<
      std::function<void(void)>
    > write_msg_queue;
    // read fiber
    boost::asio::spawn([stream = std::move(stream)](
      boost::asio::yield_context yield
    ) {
      // currently we only focus on short message
      // multi-part message is not support
      flat_buffer req_buffer;
      request req;
      try {
        while(true) {
          __http::async_read_header(stream, req_buffer, req, yield);
        }
      } catch(...) {

      }
    });
  }


  const request_router_t*     request_router_        ;
  int                         request_timeout_       ;
  int                         max_pending_req_num_   ;
};

}