#include <pixiu/server.hpp>
#include <pixiu/response.hpp>
#include "load_server_certificate.hpp"
using namespace boost::beast;
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
// https://blog.miniasp.com/post/2019/02/25/Creating-Self-signed-Certificate-using-OpenSSL
void config_logger() {
    nlohmann::json data;
    auto& loggers = data["loggers"];
    loggers["https"] = {
      {"level", "debug"}
    };
    loggers["app"] = {
      {"level", "debug"}
    };
    loggers["http_base"] = {
      {"level", "debug"}
    };
    loggers["core"] = {
      {"level", "debug"}
    };
    loggers["request_router"] = {
      {"level", "debug"}
    };
    loggers["fiber"] = {
      {"level", "debug"}
    };
    loggers["gauth"] = {
      {"level", "debug"}
    };
    pixiu::logger::config(data);
}
int main(int argc, char* argv[]) {
  try {
    config_logger();
    /**
     * make a http server and listen to 8080 port
     */
    auto server = pixiu::make_server();
    boost::asio::ssl::context ssl_ctx{ssl::context::sslv23};
    load_server_certificate(ssl_ctx);
    server.set_tls_context(ssl_ctx);
    server.get("/", [](const auto& ctx) -> pixiu::server_bits::response {
      pixiu::logger::get("app").debug("root target: {}", std::to_string(ctx.req.target()));
      return pixiu::make_response("hello world");
    });
    server.listen("0.0.0.0", 8080);
    server.run();
  } catch (const std::exception& e) {
    pixiu::logger::get("app").error(e.what());
  }
}