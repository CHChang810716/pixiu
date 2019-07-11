#pragma once
#include "macro.hpp"
#include <chrono>
namespace httpservpp {

struct config {
  static config& get() {
    static config inst = get_impl();
    return inst;
  }
private:
  static config get_impl() {
    config inst;
    inst.request_timeout_         = std::chrono::seconds(15);
    inst.use_ssl_                 = false;
    inst.port_                    = 8080;
    inst.max_pending_request_num_ = 16;
    return inst;
  }
  VMEM_GET(std::chrono::seconds, request_timeout        )
  VMEM_GET(bool,                 use_ssl                )
  VMEM_GET(unsigned short,       port                   )
  VMEM_GET(int,                  max_pending_request_num)
};

}