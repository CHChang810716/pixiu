#include <pixiu/server.hpp>
#include <pixiu/response.hpp>
#include "load_server_certificate.hpp"
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
    loggers["http_base"] = {
      {"level", "debug"}
    };
    loggers["core"] = {
      {"level", "debug"}
    };
    loggers["request_router"] = {
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
    pixiu::logger::get("app").info("{}:{}", __FILE__, __LINE__);
    boost::asio::ssl::context ssl_ctx{ssl::context::sslv23};
    pixiu::logger::get("app").info("{}:{}", __FILE__, __LINE__);
    load_server_certificate(ssl_ctx);
    pixiu::logger::get("app").info("{}:{}", __FILE__, __LINE__);
    server.set_tls_context(ssl_ctx);
    pixiu::logger::get("app").info("{}:{}", __FILE__, __LINE__);
    server.get("/", [](const auto& req) -> pixiu::server_bits::response {
      pixiu::logger::get("app").debug("root target: {}", req.target().to_string());
      // boost::beast::error_code ec;
      // http::file_body::value_type body;
      // body.open("../index.html", boost::beast::file_mode::scan, ec);
      // auto const size = body.size();
      
      // http::response<http::file_body> res{
      //   std::piecewise_construct,
      //   std::make_tuple(std::move(body)),
      //   std::make_tuple(http::status::ok, req.version())};
      // res.content_length(size);

      http::response<http::string_body> rep;
      std::string str = "hello world";
      rep.body() = str;
      rep.content_length(str.size());
      return pixiu::server_bits::response(std::move(rep));
    });
    pixiu::logger::get("app").info("{}:{}", __FILE__, __LINE__);
    server.listen("0.0.0.0", 8080);
    pixiu::logger::get("app").info("{}:{}", __FILE__, __LINE__);
    server.run();
    pixiu::logger::get("app").info("{}:{}", __FILE__, __LINE__);
  } catch (const std::exception& e) {
    pixiu::logger::get("app").error(e.what());
  }
}