#include <pixiu/server.hpp>
#include <pixiu/response.hpp>
using namespace boost::beast;
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
// https://blog.miniasp.com/post/2019/02/25/Creating-Self-signed-Certificate-using-OpenSSL
void config_logger() {
    nlohmann::json data;
    auto& loggers = data["loggers"];
    loggers["tls_http"] = {
      {"level", "debug"}
    };
    loggers["app"] = {
      {"level", "debug"}
    };
    // loggers["http_base"] = {
    //   {"level", "debug"}
    // };
    loggers["core"] = {
      {"level", "debug"}
    };
    // loggers["request_router"] = {
    //   {"level", "debug"}
    // };
    pixiu::logger::config(data);
}
int main(int argc, char* argv[]) {
  config_logger();
  /**
   * make a http server and listen to 8080 port
   */
  auto server = pixiu::make_server();
  boost::asio::ssl::context ssl_ctx{ssl::context::sslv23};
  server.set_tls_context(ssl_ctx);
  server.get("/", [](const auto& req) -> pixiu::server_bits::response {
    pixiu::logger::get("app").debug("root target: {}", req.target().to_string());
    http::response<http::string_body> rep;
    rep.body() = "hello world";
    return pixiu::server_bits::response(rep);
  });
  server.listen("0.0.0.0", 8080);
  server.run();
}