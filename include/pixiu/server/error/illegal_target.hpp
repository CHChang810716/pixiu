#pragma once
#include "base.hpp"
#include <fmt/format.h>
namespace pixiu::server::error {

struct illegal_target : public base {
  illegal_target(const std::string& name)
  : base(fmt::format("target: {} illegal", name))
  {}

  virtual response create_response(
    const request&    req
  ) const noexcept {
    return basic_error_response(__http::status::bad_request, req);
  }
};


}