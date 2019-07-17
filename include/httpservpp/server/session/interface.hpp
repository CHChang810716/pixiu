#pragma once
#include <memory>
#include <boost/lockfree/spsc_queue.hpp>
#include <httpservpp/macro.hpp>
#include <functional>
namespace httpservpp::server::session {
struct interface {
public:
  virtual void async_handle_requests() = 0;
  virtual ~interface() {}
};
using interface_ptr = std::shared_ptr<interface>;
}
namespace httpservpp::server {
  using session_ptr = session::interface_ptr;
}