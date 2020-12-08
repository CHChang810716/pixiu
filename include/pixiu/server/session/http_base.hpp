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

namespace pixiu::server_bits::session {

namespace __http    = boost::beast::http  ;
namespace __beast   = boost::beast        ;
namespace __asio    = boost::asio         ;

template<class Derived, class RequestRouter>
struct http_base 
{
private:
  using tcp_socket        = __asio::ip::tcp::socket;
  using flat_buffer       = boost::beast::flat_buffer;
  using request_router_t  = RequestRouter;
  using strand            = __asio::io_context::strand;
  static auto& logger() { return logger::get("http_base"); }
public:
  http_base(
    __asio::io_context&      ioc, 
    const request_router_t&  request_router
  ) 
  : request_router_     (&request_router)
  , timeout_            (15)
  , max_pending_req_num_(16)
  , timer_              (ioc)
  , runner_strand_        (ioc)
  , pending_req_num_    (0)
  {}
  void timeout(int sec) {
    timeout_ = sec;
  }
  void max_pending_request_num(int num) {
    max_pending_req_num_ = num;
  }
  ~http_base() {
    logger().debug("http_base destroy");
  }
protected:
  using error_code   = boost::system::error_code;
  auto derived() {
    return static_cast<Derived*>(this)->shared_from_this();
  }
  auto derived() const {
    return static_cast<const Derived*>(this)->shared_from_this();
  }
  void set_timer() {
    auto on_timeout = [__self = derived(), this](auto ec) {
      if(ec && ec != __asio::error::operation_aborted) {
        logger().error("timer error");
        return ;
      }
      if(timer_.expiry() > std::chrono::steady_clock::now() ) {
        return ;
      }
      logger().debug("do timeout process");
      __self->on_timeout();
    };
    logger().info("setup a timer");
    timer_.expires_after(
      std::chrono::seconds(timeout_)
    );
    timer_.async_wait(
      __asio::bind_executor(
        runner_strand_, std::move(on_timeout)
      )
    );
  }

  void async_run_impl(boost::asio::yield_context yield) {
      using request      = __http::request<__http::string_body>;
      // currently we only focus on short message
      // multi-part message is not support
      flat_buffer&  req_buffer = derived()->recv_buffer();
      error_code    ec          ;
      request       req         ;
      try {
        while(!derived()->is_closed()) {
          // derived()->set_timer();
          logger().debug("async_read");
          __http::async_read(derived()->stream(), req_buffer, req, yield[ec]);
          pending_req_num_ += 1;
          logger().debug("request received");
          logger().debug("{}:{}, {}", __FILE__, __LINE__, ec.message());
          // timer close socket
          if(ec == __asio::error::operation_aborted) {
            logger().debug("request operation abort");
            return;
          }
          if(ec == __http::error::end_of_stream) {
            logger().debug("request eof");
            return derived()->on_eof(yield);
          }
          if(ec)
            return logger().error("receive request failed");
          // TODO: upgrade websocket here
          logger().debug("request start process");

          request_router_->operator()(
            std::move(req), 
            [this, yield] (auto response) mutable {
              logger().debug("response created");
              return derived()->async_send_response(std::move(response), yield);
            }
          );
        }
      } catch(const std::exception& e) {
        logger().error(e.what());
      }
  }

  template<bool isRequest, class Body, class Fields>
  auto async_send_response_impl(
    __http::message<isRequest, Body, Fields>&& response, 
    boost::asio::yield_context yield
  ) {
    error_code ec;
    auto close = response.need_eof();
    logger().debug("close: {}", close);
    __http::serializer<isRequest, Body, Fields> sr{response};
    logger().debug("async_write");
    __http::async_write(derived()->stream(), sr, yield[ec]);
    this->pending_req_num_ -= 1;
    if(ec == __asio::error::operation_aborted) {
      logger().debug("response: operation aborted");
      return;
    }
    if(ec)
      return logger().error("send response failed");
    if(close) {
      logger().info("session close");
      return derived()->on_eof(yield);
    }
  }


  const request_router_t*         request_router_         ;
  int                             timeout_                ;
  int                             max_pending_req_num_    ;
  __asio::steady_timer            timer_                  ;
  strand                          runner_strand_          ;
  std::atomic_int                 pending_req_num_        ;
};

}