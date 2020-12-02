#pragma once
#include "server/core.hpp"
namespace pixiu {
template<class IOContextAR, class RequestRouterAR>
struct server
{
  // template<class T0, class T1>
  // friend server<T0, T1> make_server(T0&,T1&);
  friend struct server_maker;

  using core              = server_bits::core<IOContextAR, RequestRouterAR>;
  using core_ptr          = std::shared_ptr<core>;
  using this_t            = server<IOContextAR, RequestRouterAR>;
  using tcp               = boost::asio::ip::tcp;
  using tcp_endp          = tcp::endpoint;
  template<class Func>
  this_t& get(const std::string& target, Func&& handle) {
    impl_->request_router().get(target, std::forward<Func>(handle));
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
  template<class RequestRouter = server_bits::request_router>
  auto operator()(RequestRouter&& rr = server_bits::request_router()) const {
    using server_t = server<boost::asio::io_context, RequestRouter>;
    server_t res;
    res.impl_.reset(new typename server_t::core(
      std::forward<RequestRouter>(rr)
    ));
    return res;
  }

} make_server;

}