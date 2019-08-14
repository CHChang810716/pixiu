#include <pixiu/client/core.hpp>
#include <gtest/gtest.h>
#include <boost/beast/core/buffers_to_string.hpp>
#include <pixiu/path.hpp>
#include <iterator>

class core_test 
: public ::testing::Test 
{
protected:
  static void SetUpTestCase() {
    nlohmann::json data;
    auto& loggers = data["loggers"];
    loggers["client"] = {
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
  using namespace boost::beast;
  std::string actual;
  boost::asio::io_context ioc;
  auto core = pixiu::client::make_core(ioc);
  core->async_read(
    "www.posttestserver.com", "80", 
    11, {
      {"/", http::verb::get, {} }
    }, 
    [&actual](boost::system::error_code ec, pixiu::client::responses reps){
      actual = buffers_to_string(reps.at(0).body().data());
    }
  );
  ioc.run();
  std::ifstream fin((pixiu::path.test_data() / "client_test_site.txt").string());
  std::string expect((std::istreambuf_iterator<char>(fin)), std::istreambuf_iterator<char>());
  EXPECT_EQ(expect, actual);
}