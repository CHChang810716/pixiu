#pragma once
#include <memory>
#include <boost/lockfree/spsc_queue.hpp>
#include <httpservpp/macro.hpp>
#include <functional>
namespace httpservpp::session {
struct interface {
public:
  virtual void async_handle_requests() = 0;
};
using interface_ptr = std::shared_ptr<interface>;
}
namespace httpservpp {
  using session_ptr = session::interface_ptr;
}