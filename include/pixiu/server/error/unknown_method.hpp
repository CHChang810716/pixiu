#pragma once
#include "base.hpp"
#include <fmt/format.h>
namespace pixiu::server::error {

struct unknown_method : public base {
  unknown_method(const std::string& name)
  : base(fmt::format("unknown or not yet support HTTP method: {}", name))
  {}

  virtual response create_response(
    const request&    req
  ) const noexcept {
    return basic_error_response(__http::status::bad_request, req);
  }
};


}