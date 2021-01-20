#include <pixiu/utils.hpp>

namespace std {

std::string to_string(const std::string_view& sv) {
  return std::string{sv.data(), sv.size()};
}

}
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

}