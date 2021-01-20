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

template<class IOContextAR>
struct core 
: public std::enable_shared_from_this<core<IOContextAR>>{
protected:
    static auto& logger() {
        return pixiu::logger::get("client");
    }
public:
    using tcp           = boost::asio::ip::tcp;
    using error_code    = boost::system::error_code;
    using this_t        = core<IOContextAR>;
    
    core(IOContextAR& ioc)
    : ioc_(ioc)
    {}
    core()
    : ioc_()
    {}
    template<class ReadHandler, class ReqProc>
    BOOST_ASIO_INITFN_RESULT_TYPE(
        ReadHandler, void(error_code, responses))
    async_read(
        const std::string& host,
        const std::string& port,
        int version,
        std::vector<request_param> req_vec,
        ReadHandler&& handler,
        ReqProc&& req_proc
    ) {
        namespace http = boost::beast::http;
        BOOST_BEAST_HANDLER_INIT(
            ReadHandler, void(error_code, responses));
        boost::asio::spawn(ioc_, [
            req_vec = std::move(req_vec),
            this, __self = this->shared_from_this(),
            host, port, version,
            handler = std::move(init.completion_handler), req_proc = std::move(req_proc)
        ](boost::asio::yield_context yield){
            boost::system::error_code ec;
            tcp::resolver   resolver    {ioc_};
            tcp::socket     socket      {ioc_};
            responses       reps;
            const auto addr = resolver.async_resolve(
                host, port, yield[ec]
            );
            if(ec) {
                logger().error("resolve failed");
                return remove_const(handler)(ec, std::move(reps));
            }

            boost::asio::async_connect(socket, 
                addr.begin(), addr.end(),
                yield[ec]
            );
            if(ec) {
                logger().error("connect failed");
                return remove_const(handler)(ec, std::move(reps));
            }

            for(auto& req_param : req_vec) {
                auto req = req_param.make_request(host, version);
                req_proc(req);
                http::async_write(socket, req, yield[ec]);
                if(ec) {
                    logger().error("request failed");
                    return remove_const(handler)(ec, std::move(reps));
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
                return remove_const(handler)(ec, std::move(reps));
            }
            return remove_const(handler)(ec, std::move(reps));
        });
        return init.result.get();
    }
    
    void run() {ioc_.run();}

protected:
    IOContextAR ioc_;
};

}