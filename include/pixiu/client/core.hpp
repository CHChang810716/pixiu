#pragma once
#include <boost/beast/http.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <vector>
#include "request_param.hpp"
#include <boost/asio/spawn.hpp>
#include <string>
#include <boost/asio/ip/tcp.hpp>
#include <pixiu/logger.hpp>
#include <boost/asio/connect.hpp>

namespace pixiu::client {

struct core {
protected:
    static auto& logger() {
        return pixiu::logger::get("client");
    }
public:
    using tcp = boost::asio::ip::tcp;
    core(boost::asio::io_context& ioc)
    : ioc_(&ioc)
    {}

    void async_read(
        const std::string& host,
        const std::string& port,
        int version,
        std::vector<request_param> req_vec
    ) {
        namespace http = boost::beast::http;
        boost::asio::spawn([
            req_vec = std::move(req_vec),
            p_ioc = ioc_,
            host, port, version
        ](boost::asio::yield_context yield){
            auto& ioc = *p_ioc;
            boost::system::error_code ec;
            tcp::resolver   resolver    {ioc};
            tcp::socket     socket      {ioc};
            const auto addr = resolver.async_resolve(
                host, port, yield[ec]
            );
            if(ec) return logger().error("resolve failed");

            boost::asio::async_connect(socket, 
                addr.begin(), addr.end(),
                yield[ec]
            );
            if(ec) return logger().error("connect failed");

            for(auto& req_param : req_vec) {
                auto req = req_param.make_request(host, version);
                http::async_write(socket, req, yield[ec]);
                if(ec) return logger().error("request failed");
            }

            boost::beast::flat_buffer b;
            http::response<http::dynamic_body> rep;
        });
    }

protected:
    boost::asio::io_context* ioc_;
};

}