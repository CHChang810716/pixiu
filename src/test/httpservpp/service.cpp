#include <httpservpp/service.hpp>
#include <gtest/gtest.h>

class service_test 
: public ::testing::Test 
{
protected:
  static void SetUpTestCase() {
    httpservpp::logger::config();
  }
};

TEST_F(service_test, basic_test) {
  boost::asio::io_context ioc;
  httpservpp::service service(ioc);
  ioc.run_for(std::chrono::seconds(2));
}
TEST_F(service_test, bind_ip_test) {
  boost::asio::io_context ioc;
  httpservpp::service service(ioc);
  service.listen("127.0.0.1", 8080);
  ioc.run_for(std::chrono::seconds(2));
}