#include <gtest/gtest.h>
#include <pixiu/utils.hpp>

TEST(utils_test, split) {
  std::string_view str = "asdf;qwert;zxcvbn";
  auto splited_data = pixiu::split(str, ";");
  EXPECT_EQ(splited_data.size(), 3);
  EXPECT_EQ(splited_data[0], "asdf");
  EXPECT_EQ(splited_data[1], "qwert");
  EXPECT_EQ(splited_data[2], "zxcvbn");
}
TEST(utils_test, parse_cookie) {
  std::string_view str = "session_id=xxaaqq;user_id=johndoe;key=value";
  auto cm = pixiu::parse_cookie(str);
  EXPECT_EQ(cm.at("session_id").at(0), "xxaaqq");
  EXPECT_EQ(cm.at("user_id").at(0), "johndoe");
  EXPECT_EQ(cm.at("key").at(0), "value");

}