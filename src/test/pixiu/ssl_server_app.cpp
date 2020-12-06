#include <pixiu/server.hpp>
#include <pixiu/response.hpp>
using namespace boost::beast;
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
// https://blog.miniasp.com/post/2019/02/25/Creating-Self-signed-Certificate-using-OpenSSL
inline void load_server_certificate(boost::asio::ssl::context& ctx) {
  /*
      The certificate was generated from CMD.EXE on Windows 10 using:

      winpty openssl dhparam -out dh.pem 2048
      winpty openssl req -newkey rsa:2048 -nodes -keyout key.pem -x509 -days 10000 -out cert.pem -subj "//C=US\ST=CA\L=Los Angeles\O=Beast\CN=www.example.com"
  */

  std::string const cert =
    "-----BEGIN CERTIFICATE-----\n"
    "-----END CERTIFICATE-----\n"
  ;

  std::string const key =
    "-----BEGIN PRIVATE KEY-----\n"
    "-----END PRIVATE KEY-----\n"
  ;

  std::string const dh = 
    "-----BEGIN DH PARAMETERS-----\n"
    "-----END DH PARAMETERS-----\n"
  ;

  ctx.set_password_callback(
    [](std::size_t,
        boost::asio::ssl::context_base::password_purpose)
    {
        return "test";
    });

  ctx.set_options(
      boost::asio::ssl::context::default_workarounds |
      boost::asio::ssl::context::no_sslv2 |
      boost::asio::ssl::context::single_dh_use);

  ctx.use_certificate_chain(
    boost::asio::buffer(cert.data(), cert.size()));

  ctx.use_private_key(
    boost::asio::buffer(key.data(), key.size()),
    boost::asio::ssl::context::file_format::pem);
  ctx.use_tmp_dh(
    boost::asio::buffer(dh.data(), dh.size()));
}
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