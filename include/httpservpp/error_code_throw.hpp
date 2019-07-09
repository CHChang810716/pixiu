#pragma once
#include <boost/system/error_code.hpp>
#include <string>
namespace httpservpp {

constexpr struct ErrorCodeThrow {
  void operator()(const boost::system::error_code& ec, const std::string& msg) const {
    if(ec) throw std::runtime_error(msg);
  }
} error_code_throw;

}