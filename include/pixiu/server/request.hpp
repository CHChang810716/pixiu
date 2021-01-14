#pragma once
#include <boost/beast/http.hpp>
#include <pixiu/utils.hpp>

namespace pixiu::server_bits {


template<class Body, class Fields = boost::beast::http::fields>
struct request : public boost::beast::http::request<Body, Fields> {

  using base_t = boost::beast::http::request<Body, Fields>;
  static constexpr std::string_view session_id_key = "pixiu_session_id";

  request() = default;

  template<class... Args>
  request(Args&&... args)
  : base_t(std::forward<Args>(args)...)
  , cm_(handle_cookie(*this))
  {}

  request(request&& o)
  : base_t(std::move(o))
  , cm_(handle_cookie(*this))
  {}

  request& operator=(const request& r)  {
    static_cast<base_t&>(*this) = r;
    cm_ = handle_cookie(*this);
    return *this;
  }

  request& operator=(request&& r) {
    static_cast<base_t&>(*this) = std::move(r);
    cm_ = handle_cookie(*this);
    return *this;

  }

  const auto session_id() const {
    return cm_.at(session_id_key);
  }

  bool has_session_id() const {
    return cm_.find(session_id_key) != cm_.end();
  }

private:
  static cookie_map handle_cookie(const base_t& req) {
    auto iter =  req.find(boost::beast::http::field::set_cookie);
    cookie_map cm;
    if(iter != req.end()) {
      auto cookie_str = iter->value();
      cm = pixiu::parse_cookie(cookie_str);
    }
    return cm;
  }
  cookie_map cm_;
};

}