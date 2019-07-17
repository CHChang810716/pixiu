#include <pixiu/logger.hpp>
#include <gtest/gtest.h>
#include <pixiu/path.hpp>
TEST(logger_test, usage) {
  auto conf_path = pixiu::path .test_data() 
    / "logger_test.json";
  std::cout << conf_path << std::endl;
  pixiu::logger::config(conf_path);
  pixiu::logger::get("test_1").trace("test_1_trace");
  pixiu::logger::get("test_1").error("test_1_error");
  pixiu::logger::get("test_2").debug("test_2_debug");
  pixiu::logger::get("test_2").trace("test_2_trace");

}