#include <gtest/gtest.h>
#include <boost/beast/core/buffers_to_string.hpp>
#include <pixiu/path.hpp>
#include <iterator>
#include <pixiu/client.hpp>

class client_test 
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

TEST_F(client_test, async_read_test) {
  using namespace boost::beast;
  std::string actual;
  boost::asio::io_context ioc;
  auto client = pixiu::make_client(ioc);
  client.async_read(
    "www.posttestserver.com", "80", 
    11, {
      {"/", http::verb::get, {} }
    }, 
    [&actual](boost::system::error_code ec, pixiu::client_bits::responses reps){
      actual = buffers_to_string(reps.at(0).body().data());
    }
  );
  ioc.run();
  std::ifstream fin((pixiu::path.test_data() / "client_test_site.txt").string());
  std::string expect((std::istreambuf_iterator<char>(fin)), std::istreambuf_iterator<char>());
  EXPECT_EQ(expect, actual);
}