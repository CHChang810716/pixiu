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
    template<class CompletionToken>
    auto async_read(
        const std::string& host,
        const std::string& port,
        int version,
        std::vector<request_param> req_vec,
        CompletionToken&& token
    ) {
        namespace http = boost::beast::http;
        using response = http::response<http::dynamic_body>;
        using responses = std::vector<response>;
        boost::asio::handler_type<CompletionToken, void(responses)> handler(
            std::forward<CompletionToken>(token)
        );
        boost::asio::async_result<decltype(handler)> result(handler);
        boost::asio::spawn([
            req_vec = std::move(req_vec),
            p_ioc = ioc_,
            host, port, version,
            handler
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

            boost::beast::flat_buffer recv_buf;
            responses reps(req_param.size());
            for(auto&& rep : reps) {
                http::async_read(socket, recv_buf, rep, yield[ec]);
            }
            socket.shutdown(tcp::socket::shutdown_both, ec);

            if(ec && ec != boost::system::errc::not_connected) {
                return logger().error("connection shutdown not gracefully");
            }
            handler(std::move(reps));
        });
        return result.get();
    }

protected:
    boost::asio::io_context* ioc_;
};

}