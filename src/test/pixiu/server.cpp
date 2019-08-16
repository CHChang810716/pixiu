#include <pixiu/server/core.hpp>
#include <gtest/gtest.h>
#include <pixiu/client/core.hpp>
using namespace boost::beast;
class core_test 
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

// TEST_F(core_test, basic_test) {
//   boost::asio::io_context ioc;
//   pixiu::server_bits::core core(ioc);
//   ioc.run_for(std::chrono::seconds(2));
// }
// TEST_F(core_test, bind_ip_test) {
//   boost::asio::io_context ioc;
//   pixiu::server_bits::core core(ioc);
//   core.listen("0.0.0.0", 8080);
//   ioc.run_for(std::chrono::seconds(2));
// }
TEST_F(core_test, async_accept_test) {
  using request = pixiu::server_bits::request_router::request;
  boost::asio::io_context ioc;
  pixiu::server_bits::request_router router;
  router.get("/", [](const auto& req) -> pixiu::server_bits::response {
    http::response<http::string_body> rep;
    rep.body() = "hello world";
    return pixiu::server_bits::response(rep);
  });
  auto core = pixiu::server_bits::make_core(ioc, std::move(router));
  core->listen("0.0.0.0", 8080);
  // ioc.run();
  std::thread t([&ioc](){
    ioc.run_for(std::chrono::seconds(5));
  });

  boost::asio::io_context ioc2;
  std::string actual;
  auto client = pixiu::client_bits::make_core(ioc2);
  client->async_read(
    "localhost", "8080", 
    11, {
      {"/", http::verb::get, {} }
    }, 
    [&actual](boost::system::error_code ec, pixiu::client_bits::responses reps){
      actual = buffers_to_string(reps.at(0).body().data());
    }
  );
  ioc2.run();
  EXPECT_EQ(actual, "hello world");
  t.join();
}