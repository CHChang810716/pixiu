#pragma once
#include "base.hpp"
#include <fmt/format.h>
namespace pixiu::server::error {

struct server_error : public base {
  server_error(const std::string& what)
  : base(what)
  {}

  virtual response create_response(
    const request&    req
  ) const noexcept {
    return basic_error_response(__http::status::internal_server_error, req);
  }
};


}