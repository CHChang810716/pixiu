#include <pixiu/server.hpp>
#include <gtest/gtest.h>
#include <pixiu/client.hpp>
using namespace boost::beast;
class server_test 
: public ::testing::Test 
{
protected:
  static void SetUpTestCase() {
    nlohmann::json data;
    auto& loggers = data["loggers"];
    // loggers["plain_http"] = {
    //   {"level", "debug"}
    // };
    // loggers["http_base"] = {
    //   {"level", "debug"}
    // };
    // loggers["core"] = {
    //   {"level", "debug"}
    // };
    // loggers["request_router"] = {
    //   {"level", "debug"}
    // };
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

TEST(router_test, param_parse) {
  pixiu::server_bits::request_router router;
  router.get("/", params<int, float>("a", "b"), 
    [](const auto& req, int a, float b) -> pixiu::server_bits::response {
    }
  );
}
TEST_F(server_test, convenient_use) {
  std::string test_actual;

  auto server = pixiu::make_server();
  server.get("/", [](const auto& req) -> pixiu::server_bits::response {
    http::response<http::string_body> rep;
    rep.body() = "hello world";
    return pixiu::server_bits::response(rep);
  });
  server.listen("0.0.0.0", 8080);

  client_run(server, [&test_actual](auto& client){
    client.async_read(
      "localhost", "8080", 
      11, {
        {"/", http::verb::get, {} }
      }, 
      [&test_actual](boost::system::error_code ec, pixiu::client_bits::responses reps){
        test_actual = buffers_to_string(reps.at(0).body().data());
      }
    );
  });
  EXPECT_EQ(test_actual, "hello world");
}
// TEST_F(server_test, manual_request_router) {
//   std::this_thread::sleep_for(std::chrono::seconds(10));
//   std::string test_actual;
// 
//   boost::asio::io_context ioc;
//   pixiu::server_bits::request_router router;
//   router.get("/", [](const auto& req) -> pixiu::server_bits::response {
//     http::response<http::string_body> rep;
//     rep.body() = "hello world";
//     return pixiu::server_bits::response(rep);
//   });
//   auto core = pixiu::make_server(ioc, router);
//   core.listen("0.0.0.0", 8080);
// 
//   client_run(ioc, [&test_actual](auto& client){
//     client->async_read(
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