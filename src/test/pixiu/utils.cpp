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