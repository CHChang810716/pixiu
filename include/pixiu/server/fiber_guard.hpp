#pragma once
#include <pixiu/logger.hpp>
namespace pixiu::server_bits {

struct fiber_guard {
  fiber_guard() 
  : id (get_id()++ ) {
    logger::get("fiber").debug("spawn id: {}", id);
  }
  ~fiber_guard() {
    logger::get("fiber").debug("clear id: {}", id);
  }
private:
  static std::atomic_uint64_t& get_id() {
    static std::atomic_uint64_t id = 0;
    return id;
  }
  std::uint64_t id;
};

}