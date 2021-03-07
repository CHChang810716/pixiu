#pragma once
#include <boost/beast/http.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <vector>
#include "request_param.hpp"
#include <boost/asio/spawn.hpp>
#include <string>
#include <boost/asio/ip/tcp.hpp>
#include <pixiu/logger.hpp>
#include <boost/asio/connect.hpp>
#include <utility>
#include <boost/asio/ssl.hpp>

namespace pixiu::client_bits {

namespace __http = boost::beast::http;
namespace __asio = boost::asio;
using response = __http::response<__http::dynamic_body>;
using responses = std::vector<response>;

template<class IOContextAR>
struct https 
: public std::enable_shared_from_this<https<IOContextAR>>{
protected:
  static auto& logger() {
    return pixiu::logger::get("client");
  }
public:
  using tcp         = boost::asio::ip::tcp;
  using error_code  = boost::system::error_code;
  using this_t      = https<IOContextAR>;
  using ssl_stream  = __asio::ssl::stream<tcp::socket>;
  using ssl_context = __asio::ssl::context;
  using opt_ssl_ctx = std::optional<ssl_context>;

  https(IOContextAR& ioc)
  : ioc_(ioc)
  {}
  https()
  : ioc_()
  {}
  template<class ReadHandler, class ReqProc>
  BOOST_ASIO_INITFN_RESULT_TYPE(
    ReadHandler, void(error_code, responses))
  async_read(
    const std::string& host,
    const std::string& port,
    int version,
    std::vector<request_param> req_vec,
    ReadHandler&& handler,
    ReqProc&& req_proc
  ) {
    BOOST_BEAST_HANDLER_INIT(
      ReadHandler, void(error_code, responses));
    boost::asio::spawn(ioc_, [
      req_vec = std::move(req_vec),
      this, __self = this->shared_from_this(),
      host, port, version,
      handler = std::move(init.completion_handler), 
      req_proc = std::move(req_proc)
    ](boost::asio::yield_context yield){
      error_code      ec;
      tcp::resolver   resolver  {ioc_};
      ssl_stream      stream    {ioc_, ssl_ctx_.value()};
      responses       reps;
      if(!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())) {
        ec.assign(static_cast<int>(::ERR_get_error()), __asio::error::get_ssl_category());
        logger().error(ec.message());
        return remove_const(handler)(ec, std::move(reps));
      }
      const auto addr = resolver.async_resolve(
        host, port, yield[ec]
      );
      if(ec) {
        logger().error("resolve failed");
        return remove_const(handler)(ec, std::move(reps));
      }

      boost::asio::async_connect(stream.next_layer(), 
        addr.begin(), addr.end(),
        yield[ec]
      );
      if(ec) {
        logger().error("connect failed");
        return remove_const(handler)(ec, std::move(reps));
      }
      stream.async_handshake(
        __asio::ssl::stream_base::client,
        yield[ec]
      );
      if(ec) {
        logger().error("handshake failed");
        return remove_const(handler)(ec, std::move(reps));
      }

      for(auto& req_param : req_vec) {
        auto req = req_param.make_request(host, version);
        req_proc(req);
        logger().debug("{}", msg_to_string(req));
        __http::async_write(stream, req, yield[ec]);
        if(ec) {
          logger().error("request failed");
          return remove_const(handler)(ec, std::move(reps));
        }
      }

      boost::beast::flat_buffer recv_buf;
      reps.resize(req_vec.size());
      for(auto&& rep : reps) {
        __http::async_read(stream, recv_buf, rep, yield[ec]);
      }
      stream.async_shutdown(yield[ec]);
      if(ec) {
        // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
        if(ec == boost::system::errc::not_connected) {}
        else if(ec == boost::asio::ssl::error::stream_truncated) {}
        else if(ec == boost::asio::error::eof) {}
        else {
          logger().error("connection shutdown not gracefully: {}", ec.message());
          return remove_const(handler)(ec, std::move(reps));
        }
      }
      return remove_const(handler)(ec, std::move(reps));
    });
    return init.result.get();
  }

  void set_ssl_context(__asio::ssl::context&& ssl_ctx) {
    ssl_ctx_ = std::move(ssl_ctx);
  }
  
  void run() {ioc_.run();}
protected:
  IOContextAR ioc_;
  opt_ssl_ctx ssl_ctx_;
};

}