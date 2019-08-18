#pragma once 
#include <string>
#include <map>
#include <nlohmann/json.hpp>
#include <utility>
#include <boost/hana/tuple.hpp>
#include <boost/hana/transform.hpp>
#include <boost/hana/type.hpp>
#include <boost/lexical_cast.hpp>
namespace pixiu::server_bits {

template<class T>
using std_string = std::string; // a number trick for meta programming

template<class... T>
using name_type_tuple_t = boost::hana::tuple<
  std::pair<std_string<T>, boost::hana::type<T>>...
>;

template<class... Type>
struct params 
{
  params(const std_string<Type>&... name)
  : name_type_tuple_(std::make_pair(name, boost::hana::type<Type>())...)
  {}

  nlohmann::json parse(const boost::beast::string_view& target_str) const {
    nlohmann::json res;
    std::string_view target_view(target_str.data(), target_str.size());
    auto pos = target_view.find_first_of('?');
    auto param_str = target_view.substr(pos);
    std::map<std::string_view, std::string_view> index;
    std::size_t start_pos(0), end_pos;
    do {
      end_pos = param_str.find_first_of('&');
      auto nv_pair = param_str.substr(start_pos, end_pos);
      auto nv_split = nv_pair.find_first_of('=');
      auto name = nv_pair.substr(0, nv_split);
      auto value = nv_pair.substr(nv_split);
      index.emplace(name, value);
    } while(end_pos < param_str.size());
    auto unused = boost::hana::transform(
      name_type_tuple_, 
      [&index, &res](auto&& str_type){
        auto itr = index.find(str_type.first);
        if(itr == index.end()) return 0; 
        auto& value_str = itr->second;
        using type = typename decltype(str_type.second)::type;
        auto& name = str_type.first;
        res[name] = boost::lexical_cast<type>(value_str);
        return 0;
      }
    );
    return res;
  }

private:
  name_type_tuple_t<Type...> name_type_tuple_;
};

}