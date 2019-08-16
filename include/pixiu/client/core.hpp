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
#include <utility>

namespace pixiu::client_bits {

namespace __http = boost::beast::http;
namespace __asio = boost::asio;
using response = __http::response<__http::dynamic_body>;
using responses = std::vector<response>;

struct core {
protected:
    static auto& logger() {
        return pixiu::logger::get("client");
    }
public:
    using tcp = boost::asio::ip::tcp;
    using error_code   = boost::system::error_code;
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
        using Sig = void(error_code, responses);
        using Result = __asio::async_result<CompletionToken, Sig>;
        using Handler = typename Result::completion_handler_type;
        Handler handler(std::forward<CompletionToken>(token));
        Result result(handler);
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
            responses       reps;
            const auto addr = resolver.async_resolve(
                host, port, yield[ec]
            );
            if(ec) {
                logger().error("resolve failed");
                return handler(ec, std::move(reps));
            }

            boost::asio::async_connect(socket, 
                addr.begin(), addr.end(),
                yield[ec]
            );
            if(ec) {
                logger().error("connect failed");
                return handler(ec, std::move(reps));
            }

            for(auto& req_param : req_vec) {
                auto req = req_param.make_request(host, version);
                http::async_write(socket, req, yield[ec]);
                if(ec) {
                    logger().error("request failed");
                    return handler(ec, std::move(reps));
                }
            }

            boost::beast::flat_buffer recv_buf;
            reps.resize(req_vec.size());
            for(auto&& rep : reps) {
                http::async_read(socket, recv_buf, rep, yield[ec]);
            }
            socket.shutdown(tcp::socket::shutdown_both, ec);

            if(ec && ec != boost::system::errc::not_connected) {
                logger().error("connection shutdown not gracefully");
                return handler(ec, std::move(reps));
            }
            return handler(ec, std::move(reps));
        });
        return result.get();
    }

protected:
    boost::asio::io_context* ioc_;
};

auto make_core(
  boost::asio::io_context& ioc
) {
  return std::make_shared<core>(ioc);
}

}