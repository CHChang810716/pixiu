#pragma once
#include <boost/system/error_code.hpp>
#include <string>
namespace pixiu {

constexpr struct error_code_throw_t {
  void operator()(const boost::system::error_code& ec, const std::string& msg) const {
    if(ec) throw std::runtime_error(msg);
  }
  void operator()(const boost::system::error_code& ec) const {
    if(ec) throw std::runtime_error(ec.message());
  }
} error_code_throw;

}