#pragma once
#include "server/response.hpp"
namespace pixiu {

using response = server_bits::response;
constexpr struct MakeResponse {
  auto operator()(const std::string& str) const {
    namespace http = boost::beast::http;
    http::response<http::string_body> rep;
    rep.body() = str;
    return response(rep);
  }
} make_response;

}