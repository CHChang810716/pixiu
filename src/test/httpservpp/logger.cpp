#include <httpservpp/logger.hpp>
#include <gtest/gtest.h>
#include <httpservpp/path.hpp>
TEST(logger_test, usage) {
  auto conf_path = httpservpp::path.test_data() 
    / "logger_test.json";
  std::cout << conf_path << std::endl;
  httpservpp::logger::config(conf_path);
  httpservpp::logger::get("test_1").trace("test_1_trace");
  httpservpp::logger::get("test_1").error("test_1_error");
  httpservpp::logger::get("test_2").debug("test_2_debug");
  httpservpp::logger::get("test_2").trace("test_2_trace");

}