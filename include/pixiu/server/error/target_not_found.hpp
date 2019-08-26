#pragma once
#include "base.hpp"
#include <fmt/format.h>
namespace pixiu::server_bits::error {

struct target_not_found : public base {
  target_not_found(const std::string& name)
  : base(fmt::format("target: {} not found", name))
  {}

  virtual response create_response(
    const request&    req
  ) const noexcept {
    return basic_error_response(__http::status::not_found, req);
  }
};


}