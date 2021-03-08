#pragma once
#include "client/http.hpp"
#include "client/https.hpp"

namespace pixiu {
namespace client_bits::mode {
struct ssl {
  template<class IOContextAR>
  using core = client_bits::https<IOContextAR>;

  template<class Derived>
  struct base {
    void set_ssl_context(boost::asio::ssl::context&& ssl_ctx) {
      static_cast<Derived*>(this)->impl_->set_ssl_context(std::move(ssl_ctx));
    }
  };
};
struct plain {
  template<class IOContextAR>
  using core = client_bits::http<IOContextAR>;

  template<class Derived>
  struct base {};
};
}
template<class IOContextAR, class mode = client_bits::mode::plain>
struct client
: public mode::template base<client<IOContextAR, mode>> 
{
  friend struct client_maker;
  friend struct ssl_client_maker;
  template<class Derived>
  friend struct client_bits::mode::ssl::base;
  using request   = client_bits::request_param;
  using core      = typename mode::template core<IOContextAR>;
  using core_ptr  = std::shared_ptr<core>;
  using this_t    = client<IOContextAR>;
  
  template<class CompletionToken>
  auto async_read(
      const std::string& host,
      const std::string& port,
      int version,
      std::vector<request> req_vec,
      CompletionToken&& token
  ) {
    return impl_->async_read(host, port, version, std::move(req_vec), 
      std::forward<CompletionToken>(token), [](auto&& req){});
  }
  template<class CompletionToken, class ReqProc>
  auto async_read(
      const std::string& host,
      const std::string& port,
      int version,
      std::vector<request> req_vec,
      ReqProc&& req_proc,
      CompletionToken&& token
  ) {
    return impl_->async_read(host, port, version, std::move(req_vec), 
      std::forward<CompletionToken>(token), 
      std::forward<ReqProc>(req_proc)
    );
  }
  this_t& run() { impl_->run(); }

private:
  core_ptr impl_;
};

constexpr struct client_maker {
  template<class IOContext>
  auto operator()(IOContext& ioc) const {
    using client_t = client<IOContext&>;
    client_t res;
    res.impl_.reset(new typename client_t::core(ioc));
    return res;
  }
  auto operator()() const {
    using client_t = client<boost::asio::io_context>;
    client_t res;
    res.impl_.reset(new typename client_t::core());
    return res;
  }

} make_client;

constexpr struct ssl_client_maker {
  template<class IOContext>
  auto operator()(IOContext& ioc) const {
    using client_t = client<IOContext&, client_bits::mode::ssl>;
    client_t res;
    res.impl_.reset(new typename client_t::core(ioc));
    return res;
  }
  auto operator()() const {
    using client_t = client<boost::asio::io_context, client_bits::mode::ssl>;
    client_t res;
    res.impl_.reset(new typename client_t::core());
    return res;
  }

} make_ssl_client;

template<class IOC>
using ssl_client = client<IOC, client_bits::mode::ssl>;
}