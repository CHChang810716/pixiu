#include <pixiu/server.hpp>
#include <pixiu/response.hpp>
#include "load_server_certificate.hpp"
#include <pixiu/gauth.hpp>
#include <pixiu/request_router.hpp>
using namespace boost::beast;
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
// https://blog.miniasp.com/post/2019/02/25/Creating-Self-signed-Certificate-using-OpenSSL
void config_logger() {
    nlohmann::json data;
    auto& loggers = data["loggers"];
    loggers["http"] = {
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
    loggers["client"] = {
      {"level", "debug"}
    };
    pixiu::logger::config(data);
}
struct session {
  nlohmann::json google_auth;
};
auto& logger() {
  return pixiu::logger::get("app");
}
int main(int argc, char* argv[]) {
  boost::asio::io_context ioc;
  pixiu::request_router<session> router;
  try {
    config_logger();
    pixiu::gauth_gconfig gauth_cfg {
      // client_id,
      // auth_uri,
      // token_uri,
      // client_secret,
    };
    /**
     * make a http server and listen to 8080 port
     */
    auto server = pixiu::make_server(ioc, router);
    logger().info("{}:{}", __FILE__, __LINE__);
    server.get("/gauth", 
      pixiu::gauth_gconfig::params(),
      [&ioc, &gauth_cfg](const auto& ctx, const pixiu::opt_str& code) {
        auto gauth = pixiu::make_gauth(
          ioc, gauth_cfg,
          "http://localhost:8080/gauth", {
            "openid"
          }
        );
        return gauth.callback(ctx, code);
      }
    );
    server.get("/.+", [](const auto& ctx) -> pixiu::server_bits::response {
      logger().debug("root target: {}", std::to_string(ctx.req.target()));
      return pixiu::make_response("hello world");
    });
    server.get("/", [](const auto& ctx) -> pixiu::server_bits::response {
      logger().debug("root target: {}", std::to_string(ctx.req.target()));
      if(!ctx.session().google_auth.is_null()) {
        logger().debug("google auth data found!!!");
      } else {
        logger().debug("google auth data not ready...");
      }
      return pixiu::make_response("root");
    });
    server.listen("0.0.0.0", 8080);
    server.run();
  } catch (const std::exception& e) {
    logger().error(e.what());
  }
}