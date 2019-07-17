#pragma once
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http.hpp>
#include <pixiu/config.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/bind_executor.hpp>
#include <pixiu/logger.hpp>
#include <pixiu/server/request_handler.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace pixiu::server::session {

namespace __http    = boost::beast::http  ;
namespace __beast   = boost::beast        ;
namespace __asio    = boost::asio         ;

template<class RequestHandler>
struct http_base 
{
private:
  using tcp_socket        = __asio::ip::tcp::socket;
  using flat_buffer       = boost::beast::flat_buffer;
  using request_handler_t = RequestHandler;
  static auto& logger() { return logger::get("http_base"); }
public:
  http_base(
    __asio::io_context&       ioc, 
    const request_handler_t&  request_handler
  ) 
  : req_timer_          (ioc)
  , req_                ()
  , req_queue_          (ioc.get_executor())
  , rep_queue_          (ioc.get_executor())
  , pending_req_num_    (0)
  , request_handler_    (&request_handler)
  , request_timeout_    (15)
  , max_pending_req_num_(16)
  {}
  ~http_base() {
    logger().debug("http_base destroy");
  }
protected:
  using request_body = __http::request<__http::string_body>;
  using strand       = __asio::strand<__asio::io_context::executor_type>;
  using error_code   = boost::system::error_code;
private:
  template<class Func>
  decltype(auto) make_seq_req_handler(Func&& func) {
    return __asio::bind_executor(
      req_queue_, std::forward<Func>(func)
    );
  }
  template<class Func>
  decltype(auto) make_seq_rep_handler(Func&& func) {
    return __asio::bind_executor(
      rep_queue_, std::forward<Func>(func)
    );
  }
  void request_timeout(int sec) {
    request_timeout_ = sec;
  }
  void max_pending_request_num(int num) {
    max_pending_req_num_ = num;
  }
protected:
  template<class Derived>
  void set_request_timeout_handler(Derived& derived) {
    auto on_timeout = [derived, this](auto ec) {
      logger().debug("request timeout process");
      if(ec && ec != __asio::error::operation_aborted) {
        logger().error("timer error");
        return ;
      }
      if(req_timer_.expiry() > std::chrono::steady_clock::now() ) {
        return ;
      }
      derived->do_request_timeout();
    };
    req_timer_.async_wait(
      make_seq_req_handler(on_timeout)
    );
    req_timer_.expires_after(
      std::chrono::seconds(request_timeout_)
    );
  }
  
  template<class Http>
  void async_recv_request(Http derived) {
    logger().debug("start wait request");
    auto on_recv_request = [derived, this](error_code ec, std::size_t bytes_transf) -> void {
      logger().debug("request received");
      // timer close socket
      if(ec == __asio::error::operation_aborted) {
        logger().debug("request operation abort");
        return;
      }
      if(ec == __http::error::end_of_stream) {
        logger().debug("request eof");
        return derived->do_eof();
      }
      if(ec)
        return logger().error("receive request failed");
      // TODO: upgrade websocket here

      logger().debug("request start process");
      this->pending_req_num_ += 1;

      request_handler_->operator()(
        std::move(derived->req_), 
        [derived](auto response) {
          logger().debug("response created");
          derived->async_send_response(std::move(response));
        }
      );
      this->async_recv_request(derived);
    };
    if(this->pending_req_num_ > max_pending_req_num_) {
      // prevent ddos attack
      return ;
    }
    set_request_timeout_handler(derived);
    __http::async_read(
      derived->stream(), 
      derived->recv_buffer(), 
      req_, 
      make_seq_req_handler(on_recv_request)
    );
  }
  template<class Http, bool isRequest, class Body, class Fields>
  void async_send_response(
    Http derived, 
    __http::message<isRequest, Body, Fields> msg
  ) {
    auto on_send_response = 
    [derived, this](error_code ec, bool close) {
      if(ec == __asio::error::operation_aborted) {
        logger().debug("response: operation aborted");
        return;
      }
      if(ec)
        return logger().error("send response failed");
      if(close) {
        logger().debug("response: do close");
        return derived->do_eof();
      }

      this->pending_req_num_ -= 1;
    };
    boost::asio::post(rep_queue_, [
      msg = std::move(msg), 
      derived, this, 
      on_send_response
    ](){
      boost::system::error_code ec;
      bool close = msg.need_eof();
      __http::serializer<isRequest, Body, Fields> sr(std::move(msg));
      logger().debug("response sending");
      __http::write(derived->stream(), sr, ec);
      logger().debug("response sended");
      boost::asio::post(
        req_queue_, std::bind(on_send_response, ec, close)
      );
    });
  }
protected:
  boost::asio::steady_timer   req_timer_             ;
  boost::beast::flat_buffer   recv_buffer_           ;
  request_body                req_                   ; // workaroud for seperated message
  strand                      req_queue_             ;
  strand                      rep_queue_             ;
  std::atomic_int             pending_req_num_       ;
  const request_handler_t*    request_handler_       ;
  int                         request_timeout_       ;
  int                         max_pending_req_num_   ;
};

}