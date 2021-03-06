#pragma once
#include "server/response.hpp"
#include <boost/filesystem.hpp>
#include <string_view>
#include <nlohmann/json.hpp>

namespace pixiu {

using response = server_bits::response;
constexpr struct MakeResponse {
  auto operator()(const char* str) const {
    return string_view_impl(str);
  }
  auto operator()(const std::string& str) const {
    return string_view_impl(str);
  }
  auto operator()(const std::string_view& str) const {
    return string_view_impl(str);
  }
  auto operator()(const boost::filesystem::path& path) const {
    namespace http = boost::beast::http;
    boost::beast::error_code ec;
    http::file_body::value_type body;
    body.open(path.string().c_str(), boost::beast::file_mode::scan, ec);
    auto const size = body.size();
    http::response<http::file_body> res{
      std::piecewise_construct,
      std::make_tuple(std::move(body))
    };
    res.content_length(size);
    return response(std::move(res));
  }
  auto operator()(const nlohmann::json& json) const {
    return string_view_impl(json.dump());
  }
private:
  response string_view_impl(const std::string_view& str) const {
    namespace http = boost::beast::http;
    http::response<http::string_body> rep;
    rep.body() = str;
    rep.content_length(str.length());
    return response(std::move(rep));
  }
} make_response;

constexpr struct MakeRedirect {
  auto operator()(const std::string& uri) const {
    namespace http = boost::beast::http;
    http::response<http::empty_body> rep;
    rep.result(302);
    rep.set(http::field::location, uri);
    return response(rep);
  }
} make_redirect;

}