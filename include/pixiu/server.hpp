#pragma once
#include "server/core.hpp"
#include <pixiu/server/request_router.hpp>
#include <pixiu/server/session_request_router.hpp>

namespace pixiu {
template<class IOContextAR, class RequestRouterAR>
struct server
{
  friend struct server_maker;

  using core              = server_bits::core<IOContextAR, RequestRouterAR>;
  using core_ptr          = std::shared_ptr<core>;
  using this_t            = server<IOContextAR, RequestRouterAR>;
  using tcp               = boost::asio::ip::tcp;
  using tcp_endp          = tcp::endpoint;

  template<class... Args>
  this_t& get(
    Args&&... args
  ) {
    impl_->request_router().get(std::forward<Args>(args)...);
    return *this;
  }

  this_t& listen(tcp_endp ep) {
    impl_->listen(ep);
    return *this;
  }

  this_t& listen( const std::string& ip, unsigned short port) {
    impl_->listen(ip, port);
    return *this;
  }

  void run() { impl_->run(); }

  template<class DU>
  this_t& run_for(DU&& du) { 
    impl_->run_for(du); 
    return *this;
  }
  void set_tls_context(boost::asio::ssl::context& ctx) {
    impl_->set_tls_context(ctx);
  }


private:
  core_ptr impl_;
};
constexpr struct server_maker {

  template<class IOContext, class RequestRouter>
  auto operator()(IOContext& ioc, RequestRouter&& rr) const {
    using server_t = server<IOContext&, RequestRouter>;
    server_t res;
    res.impl_.reset(new typename server_t::core(
      ioc, std::forward<RequestRouter>(rr)));
    return res;
  }
  template<class RequestRouter = server_bits::session_request_router>
  auto operator()(RequestRouter&& rr = server_bits::session_request_router()) const {
    using server_t = server<boost::asio::io_context, RequestRouter>;
    server_t res;
    res.impl_.reset(new typename server_t::core(
      std::forward<RequestRouter>(rr)
    ));
    return res;
  }

} make_server;

}