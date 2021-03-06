#pragma once
#include <memory>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <regex>
#include <exception>
#include <pixiu/logger.hpp>
#include <pixiu/utils.hpp>
#include <pixiu/server/request.hpp>
#include <random>
#include <nlohmann/json.hpp>
#include <boost/asio/spawn.hpp>

namespace pixiu::server_bits::session {

namespace __http    = boost::beast::http  ;
namespace __beast   = boost::beast        ;
namespace __asio    = boost::asio         ;

template<class Session = nlohmann::json>
struct context {
  using request     = pixiu::server_bits::request<__http::string_body>;
  using session_t   = Session;
  using g_session_t = std::map<std::string, session_t>;

  context(boost::asio::yield_context y)
  : yield_(y)
  {}

  context() = default;

  session_t& session() const {
    if(!sid.empty()) {
      return g_session_data()[std::string{sid.begin(), sid.end()}];
    }
    if(req.has_session_id()) {
      if(g_session_data().find(req.session_id()) != g_session_data().end()) {
        const_cast<std::string&>(sid) = req.session_id();
        return g_session_data()[std::string{sid.begin(), sid.end()}];
      }
    }
    auto& g_s = g_session_data();

    while(true) {
      auto _sid = random_str(16);
      if(g_s.find(_sid) == g_s.end()) {
        const_cast<std::string&>(sid) = _sid;
        break;
      }
    }
    return g_session_data()[std::string{sid.begin(), sid.end()}];
  }
  auto& get_yield_ctx() const { return yield_.value(); }

  request                   req;
  std::string               sid;
  std::vector<std::string>  url_capt;

  static g_session_t& g_session_data() {
    static g_session_t data;
    return data;
  }
private:
  std::optional<boost::asio::yield_context> yield_;
  static std::string __random_char_pool() {
    std::string str;
    for(char c = 'a'; c < 'z'; c++) {
      str.push_back(c);
    }
    for(char c = 'A'; c < 'Z'; c++) {
      str.push_back(c);
    }
    for(char c = '0'; c < '9'; c++) {
      str.push_back(c);
    }
    return str;
  }
  static std::string& random_char_pool() {
    static std::string data = __random_char_pool();
    return data;
  }
  static std::string random_str(int len) {
    static auto& char_pool = random_char_pool();
    static std::default_random_engine rng(std::random_device{}());
    static std::uniform_int_distribution<> dist(0, char_pool.size() - 1);
    std::string res;
    res.resize(len);
    for(int i = 0; i < len; i ++) {
      res[i] = char_pool[dist(rng)];
    }
    return res;
  }
};
}