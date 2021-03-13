#pragma once
#include <boost/beast/http.hpp>
#include <pixiu/utils.hpp>

namespace pixiu::server_bits {


template<class Body, class Fields = boost::beast::http::fields>
struct request : public boost::beast::http::request<Body, Fields> {

  using base_t = boost::beast::http::request<Body, Fields>;
  static constexpr const char* session_id_key = "pixiu_session_id";

  using base_t::base_t;

  const auto session_ids() const {
    return cm_.at(session_id_key);
  }

  bool has_session_id() const {
    return cm_.find(session_id_key) != cm_.end();
  }

  void build_ext() {
    cm_ = handle_cookie(*this);
  }

private:
  static cookie_map handle_cookie(const base_t& req) {
    auto iter =  req.find(boost::beast::http::field::cookie);
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