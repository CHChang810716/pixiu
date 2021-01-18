#include <pixiu/server.hpp>
#include <gtest/gtest.h>
#include <pixiu/client.hpp>
#include <pixiu/response.hpp>
#include <pixiu/request_utils.hpp>
#include <pixiu/request_router.hpp>
#include <thread>

using namespace boost::beast;
class server_test 
: public ::testing::Test 
{
protected:
  static void SetUpTestCase() {
    nlohmann::json data;
    auto& loggers = data["loggers"];
    // loggers["http"] = {
    //   {"level", "debug"}
    // };
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
};
template<class ServIOC, class Func>
auto client_run(ServIOC& serv_ioc, Func&& func) {
  std::thread t([&serv_ioc](){
    serv_ioc.run_for(std::chrono::seconds(5));
  });
  boost::asio::io_context ioc2;
  auto client = pixiu::make_client(ioc2);
  func(client);
  ioc2.run();
  t.join();
}
template<class... T>
using params = pixiu::server_bits::params<T...>;
TEST(params_test, parse) {
  params<int, float, std::string, std::uint16_t> parser(
    "integer", "float32", "str", "uint_16bit"
  );
  auto tuple = parser.parse("www.asdf.com/mysite?integer=100&float32=3.414&str=qsefth&uint_16bit=65535");
  EXPECT_EQ(boost::hana::at_c<0>(tuple), int(100));
  EXPECT_TRUE(std::abs(boost::hana::at_c<1>(tuple) - float(3.414)) < 0.0001);
  EXPECT_EQ(boost::hana::at_c<2>(tuple), "qsefth");
  EXPECT_EQ(boost::hana::at_c<3>(tuple), std::uint16_t(65535));
}

TEST_F(server_test, router_call) {
  pixiu::request_router router;
  router.get("/", params<int, float>("a", "b"), 
    [](const auto& req, int a, float b) {
      return pixiu::make_response(std::to_string(a + b));
    }
  );
  pixiu::server_bits::session_storage session;
  session.req = pixiu::make_request(
    http::verb::get,
    "localhost:8080","/", 
    11, {
      {"a", 12},
      {"b", 2.4}
    }
  );

  router(session, [](auto&& rep){
    using Rep = std::decay_t<decltype(rep)>;
    if constexpr(std::is_same_v<typename Rep::body_type, http::string_body>) {
      auto actual = std::stod(rep.body());
      EXPECT_TRUE(std::abs(actual - 14.4) < 0.001);
    }
  });
}
// TEST_F(server_test, convenient_use) {
//   std::string test_actual;
// 
//   auto server = pixiu::make_server();
//   server.get("/", [](const auto& req) -> pixiu::server_bits::response {
//     http::response<http::string_body> rep;
//     rep.body() = "hello world";
//     return pixiu::server_bits::response(rep);
//   });
//   server.listen("0.0.0.0", 8080);
// 
//   client_run(server, [&test_actual](auto& client){
//     client.async_read(
//       "localhost", "8080", 
//       11, {
//         {"/", http::verb::get, {} }
//       }, 
//       [&test_actual](boost::system::error_code ec, pixiu::client_bits::responses reps){
//         test_actual = buffers_to_string(reps.at(0).body().data());
//       }
//     );
//   });
//   EXPECT_EQ(test_actual, "hello world");
// }
TEST_F(server_test, manual_request_router) {
  std::string test_actual;

  pixiu::request_router router;
  router.get("/", params<int, float>("a", "b"), 
    [](const auto& session, int a, float b) {
      return pixiu::make_response(std::to_string(a + b));
    }
  );
  router.get("/session_id", 
    [](const auto& session) {
      session.session();
      return pixiu::make_response(session.sid);
    }
  );

  auto server = pixiu::make_server(router);
  server.listen("0.0.0.0", 8080);

  client_run(server, [&test_actual](auto& client){
    client.async_read(
      "localhost", "8080", 
      11, {
        {"/", http::verb::get, {
          {"a", 12},
          {"b", 2.4}
        } }
      }, 
      [&test_actual](boost::system::error_code ec, pixiu::client_bits::responses reps){
        test_actual = buffers_to_string(reps.at(0).body().data());
      }
    );
    client.async_read(
      "localhost", "8080", 
      11, {
        {"/session_id", http::verb::get}
      }, 
      [&test_actual](boost::system::error_code ec, pixiu::client_bits::responses reps){
        auto str = buffers_to_string(reps.at(0).body().data());
        auto set_cookie = reps.at(0)[boost::beast::http::field::set_cookie];
        std::cout << set_cookie << std::endl;
        EXPECT_GT(str.size(), 0);
        EXPECT_GT(set_cookie.size(), 0);
      }
    );
    auto req = pixiu::make_request(
      http::verb::get,
      "localhost:8080","/session_id", 
      11, {}
    );
    req.set(http::field::cookie, "pixiu_session_id:qsefthuk90");
    client.async_read(
      "localhost", "8080", 
      11, {req}, 
      [&test_actual](boost::system::error_code ec, pixiu::client_bits::responses reps){
        std::cout << reps.at(0) << std::endl;
      }
    );
  });
  EXPECT_LT(std::abs(std::stod(test_actual) - 14.4), 0.01);
}