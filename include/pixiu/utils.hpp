#pragma once
#include <map>
#include <string_view>
#include <vector>

namespace pixiu {

std::vector<std::string_view> split(std::string_view str, const std::string_view& delim) {
  std::vector<std::string_view> result;
  for(std::size_t i = str.find(delim); 
    i < str.size(); 
    i = str.find(delim)
  ) {
    std::string_view entry{str.data(), i};
    result.push_back(entry);
    str = str.substr(i + 1);
  }
  if(str.size() > 0) {
    result.push_back(str);
  }
  return result;
  
}
using cookie_map = std::map<
  std::string_view, 
  std::string_view
>;
template<class Str>
cookie_map parse_cookie(Str&& str) {
  cookie_map res;
  auto sstr = split(std::forward<Str>(str), ";");
  for(auto&& entry : sstr) {
    auto as_s = entry.find("=");
    auto key = entry.substr(0, as_s);
    auto value = entry.substr(as_s + 1);
    res[key] = value;
  }
  return res;
}

  
} // namespace pixiu::server
