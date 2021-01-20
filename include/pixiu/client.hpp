#pragma once
#include "client/core.hpp"

namespace pixiu {

template<class IOContextAR>
struct client {
  friend struct client_maker;
  using request = client_bits::request_param;
  using core = client_bits::core<IOContextAR>;
  using core_ptr = std::shared_ptr<core>;
  using this_t = client<IOContextAR>;
  
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
}