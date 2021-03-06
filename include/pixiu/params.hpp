#pragma once
#include <pixiu/server/params.hpp>
namespace pixiu {

template<class... T>
using params = server_bits::params<T...>;

}