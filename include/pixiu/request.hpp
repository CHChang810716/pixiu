#pragma once

#include "server/request.hpp"

namespace pixiu {

template<class Body, class Fields = boost::beast::http::fields>
using request = server_bits::request<Body, Fields>;

}