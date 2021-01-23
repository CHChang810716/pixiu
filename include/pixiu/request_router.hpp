#pragma once
// #include "server/request_router.hpp"
#include "server/request_router.hpp"
namespace pixiu {

template<class... Args>
using request_router = server_bits::request_router<Args...>;

}