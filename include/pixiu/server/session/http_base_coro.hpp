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
  , read_strand_        (ioc)
  , write_strand_       (ioc)
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
      logger().debug("request timeout process");
      if(ec && ec != __asio::error::operation_aborted) {
        logger().error("timer error");
        return ;
      }
      if(timer_.expiry() > std::chrono::steady_clock::now() ) {
        return ;
      }
      __self->on_timeout();
    };
    timer_.expires_after(
      std::chrono::seconds(timeout_)
    );
    timer_.async_wait(
      __asio::bind_executor(
        read_strand_, std::move(on_timeout)
      )
    );
  }

  void async_recv_requests() {
    using request      = __http::request<__http::string_body>;
    // read fiber
    boost::asio::spawn(read_strand_, [__self = derived(), this](
      boost::asio::yield_context yield
    ) {
      // currently we only focus on short message
      // multi-part message is not support
      flat_buffer   req_buffer  ;
      request       req         ;
      error_code    ec          ;
      try {
        while(true) {
          // if(writer_msg_queue_.write_available() <= 0) continue;
          __self->set_timer();
          __http::async_read(__self->stream(), req_buffer, req, yield[ec]);
          pending_req_num_ += 1;
          logger().debug("request received");
          // timer close socket
          if(ec == __asio::error::operation_aborted) {
            logger().debug("request operation abort");
            return;
          }
          if(ec == __http::error::end_of_stream) {
            logger().debug("request eof");
            return __self->on_eof();
          }
          if(ec)
            return logger().error("receive request failed");
          // TODO: upgrade websocket here
          logger().debug("request start process");

          request_router_->operator()(
            std::move(req), 
            [__self, this] (auto response) mutable {
              logger().debug("response created");
              __self->async_send_response(std::move(response));
            }
          );

          // TODO: make multi-part message work from here
          // if(/* is multi-part message */false) {
          // } else {
          //   __self->set_timer();
          //   __http::async_read(__self->stream(), req_buffer, req, yield);
          // }
        }
      } catch(const std::exception& e) {
        logger().error(e.what());
      }
    });
  }
  template<class Rep>
  void async_send_response(Rep response) {
    boost::asio::post(
      write_strand_, 
      [
        __self = derived(), this, 
        response = std::move(response)
      ]() mutable {
        error_code ec;
        auto close = response.need_eof();
        __http::write(__self->stream(), response, ec);
        pending_req_num_ -= 1;
        if(ec == __asio::error::operation_aborted) {
          logger().debug("response: operation aborted");
          return;
        }
        if(ec)
          return logger().error("send response failed");
        if(close) {
          logger().debug("response: do close");
          return __self->on_eof();
        }
      }
    );
  }


  const request_router_t*         request_router_         ;
  int                             timeout_                ;
  int                             max_pending_req_num_    ;
  __asio::steady_timer            timer_                  ;
  strand                          read_strand_            ;
  strand                          write_strand_           ;
  std::atomic_int                 pending_req_num_        ;

};

}