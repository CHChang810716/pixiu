#pragma once
#include "client/request_param.hpp"

namespace pixiu {

using request_param = client_bits::request_param;
constexpr struct MakeRequest {
  auto operator()(
    const boost::beast::http::verb method,
    const std::string& host,
    boost::string_view target, 
    int version,
    nlohmann::json param
  ) const {
    request_param rp;
    rp.target = target;
    rp.method = method;
    rp.param = std::move(param);
    return rp.make_request(host, version);
  }
} make_request;

}