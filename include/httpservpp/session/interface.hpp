#pragma once
#include <memory>
namespace httpservpp::session {
struct interface {
  
};
using interface_ptr = std::shared_ptr<interface>;
}
namespace httpservpp {
  using session_ptr = session::interface_ptr;
}