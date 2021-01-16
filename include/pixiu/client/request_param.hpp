#pragma once
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>
#include <boost/beast/version.hpp>
#include <fmt/format.h>
#include <pixiu/request.hpp>
namespace pixiu::client_bits {

struct request_param {
    std::string_view            target  ;
    boost::beast::http::verb    method  ;
    nlohmann::json              param   ;
    auto make_request(
        const std::string& host, 
        int version
    ) const {
        namespace http = boost::beast::http;
        request<http::string_body> request;
        request.version(version);
        request.method(method);
        request.set(http::field::host, host);
        request.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        std::string uri{target.begin(), target.end()};
        char spliter = '?';
        for(auto itr = param.begin(); itr != param.end(); itr++) {
            uri += spliter;
            uri += fmt::format("{}={}", itr.key(), itr.value().dump());
            spliter = '&';
        }
        request.target(uri);
        return request;
    }
};

}