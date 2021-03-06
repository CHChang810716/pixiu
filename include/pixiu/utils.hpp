#pragma once
#include <map>
#include <string_view>
#include <vector>
#include <string>
#include <type_traits>
#include <sstream>
#include <optional>
namespace std {

std::string to_string (const std::string_view& sv);

}
namespace pixiu {

template<class Msg>
auto msg_to_string(Msg&& msg) {
  std::stringstream ss;
  ss << msg << '\n';
  return ss.str();
}

template<class T>
auto& remove_const(T& o) {
  using RCT = std::remove_cv_t<T>;
  return const_cast<RCT&>(o);
}

std::vector<std::string_view> split(std::string_view str, const std::string_view& delim);

using cookie_map = std::map<
  std::string, 
  std::string
>;
template<class Str>
cookie_map parse_cookie(Str&& str) {
  cookie_map res;
  auto sstr = split(std::forward<Str>(str), ";");
  for(auto&& entry : sstr) {
    auto as_s = entry.find("=");
    auto key = entry.substr(0, as_s);
    auto value = entry.substr(as_s + 1);
    res[std::to_string(key)] = std::to_string(value);
  }
  return res;
}

using opt_str = std::optional<std::string>;

template<class T> struct type_wrapper {};
template<class T> struct remove_optional { using type = T; };
template<class T> struct remove_optional<std::optional<T>> { using type = T; };
template<class T> using remove_optional_t = typename remove_optional<T>::type;
  
} // namespace pixiu
