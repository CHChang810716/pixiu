#include <pixiu/client/core.hpp>
#include <gtest/gtest.h>

class core_test 
: public ::testing::Test 
{
protected:
  static void SetUpTestCase() {
    nlohmann::json data;
    auto& loggers = data["loggers"];
    loggers["plain_http"] = {
      {"level", "debug"}
    };
    loggers["http_base"] = {
      {"level", "debug"}
    };
    loggers["core"] = {
      {"level", "debug"}
    };
    loggers["request_handler"] = {
      {"level", "debug"}
    };
    pixiu::logger::config(data);
  }
};

// TEST_F(core_test, basic_test) {
//   boost::asio::io_context ioc;
//   pixiu::server::core core(ioc);
//   ioc.run_for(std::chrono::seconds(2));
// }
// TEST_F(core_test, bind_ip_test) {
//   boost::asio::io_context ioc;
//   pixiu::server::core core(ioc);
//   core.listen("127.0.0.1", 8080);
//   ioc.run_for(std::chrono::seconds(2));
// }
TEST_F(core_test, async_read_test) {
  boost::asio::io_context ioc;
  auto core = pixiu::client::make_core(ioc);
  core->async_read(
    "127.0.0.1", "8080", 
    11, {}, 
    [](pixiu::client::responses reps){

    }
  );
  ioc.run();
}