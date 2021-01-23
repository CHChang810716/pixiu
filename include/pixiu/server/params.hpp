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

  auto parse_get_url(const std::string_view& target_str) const {
    std::map<std::string_view, std::string_view> index;
    std::string_view target_view(target_str.data(), target_str.size());
    auto pos = target_view.find_first_of('?');
    if(pos < (target_view.size() - 1)) {
      auto param_str = target_view.substr(pos + 1);
      
      std::size_t end_pos = 0;
      do {
        end_pos = param_str.find_first_of('&');
        auto nv_pair = param_str.substr(0, end_pos);
        auto nv_split = nv_pair.find_first_of('=');
        auto name = nv_pair.substr(0, nv_split);
        if(nv_split < (nv_pair.size() - 1)) {
          auto value = nv_pair.substr(nv_split + 1);
          index.emplace(name, value);
        }
        if(end_pos < (param_str.size() - 1))
          param_str = param_str.substr(end_pos + 1);
        else break;
      } while(true);
    }

    auto res = boost::hana::transform(
      name_type_tuple_, 
      [&index](auto&& str_type){
        using type = typename decltype(str_type.second)::type;
        auto itr = index.find(str_type.first);
        if(itr == index.end()) return type(); 
        auto& value_str = itr->second;
        return boost::lexical_cast<type>(value_str); // TODO: should have a better solution
      }
    );
    return res;
  }

  auto parse_post_body(const std::string& body) const {
    auto json_param = nlohmann::json::parse(body);
    auto res = boost::hana::transform(
      name_type_tuple_, 
      [&json_param](auto&& str_type){
        using type = typename decltype(str_type.second)::type;
        auto itr = json_param.find(str_type.first);
        if(itr == json_param.end()) return type(); 
        return itr->template get<type>();
      }
    );
    return res;
  }
private:
  name_type_tuple_t<Type...> name_type_tuple_;
};

}