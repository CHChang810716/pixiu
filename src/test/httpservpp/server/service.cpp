#include <httpservpp/server/service.hpp>
#include <gtest/gtest.h>

class service_test 
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
    loggers["service"] = {
      {"level", "debug"}
    };
    loggers["request_handler"] = {
      {"level", "debug"}
    };
    httpservpp::logger::config(data);
  }
};

// TEST_F(service_test, basic_test) {
//   boost::asio::io_context ioc;
//   httpservpp::server::service service(ioc);
//   ioc.run_for(std::chrono::seconds(2));
// }
// TEST_F(service_test, bind_ip_test) {
//   boost::asio::io_context ioc;
//   httpservpp::server::service service(ioc);
//   service.listen("127.0.0.1", 8080);
//   ioc.run_for(std::chrono::seconds(2));
// }
TEST_F(service_test, async_accept_test) {
  boost::asio::io_context ioc;
  auto service = httpservpp::server::make_service(ioc);
  service->listen("127.0.0.1", 8080);
  service->async_accept();
  ioc.run();
}