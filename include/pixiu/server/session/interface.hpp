#pragma once
#include <memory>
#include <boost/lockfree/spsc_queue.hpp>
#include <pixiu/macro.hpp>
#include <functional>
namespace pixiu::server_bits::session {
struct interface {
public:
  virtual void spawn() = 0;
  virtual bool is_closed() const = 0;
  virtual ~interface() {}
};
using interface_ptr = std::shared_ptr<interface>;
}
namespace pixiu::server_bits {
  using session_ptr = session::interface_ptr;
}