#pragma once
#include <memory>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include "response.hpp"
#include <regex>
#include "error/all.hpp"
#include <exception>
#include "params.hpp"
#include <pixiu/logger.hpp>
#include <pixiu/utils.hpp>
#include <pixiu/server/request.hpp>
#include <random>
#include <nlohmann/json.hpp>
#include "session/context.hpp"


namespace pixiu::server_bits {

namespace __http    = boost::beast::http  ;
namespace __beast   = boost::beast        ;
namespace __asio    = boost::asio         ;

// struct session_storage {
//   using request = pixiu::server_bits::request<__http::string_body>;
//   nlohmann::json& session() const {
//     if(!sid.empty()) {
//       return g_session_data()[std::string{sid.begin(), sid.end()}];
//     }
//     if(req.has_session_id()) {
//       if(g_session_data().find(req.session_id()) != g_session_data().end()) {
//         const_cast<std::string&>(sid) = req.session_id();
//         return g_session_data()[std::string{sid.begin(), sid.end()}];
//       }
//     }
//     auto& g_s = g_session_data();
// 
//     while(true) {
//       auto _sid = random_str(16);
//       if(g_s.find(_sid) == g_s.end()) {
//         const_cast<std::string&>(sid) = _sid;
//         break;
//       }
//     }
//     return g_session_data()[std::string{sid.begin(), sid.end()}];
// 
//   }
//   request req;
//   std::string sid;
//   std::vector<std::string> url_capt;
// 
//   static nlohmann::json& g_session_data() {
//     static nlohmann::json data;
//     return data;
//   }
// private:
//   static std::string __random_char_pool() {
//     std::string str;
//     for(char c = 'a'; c < 'z'; c++) {
//       str.push_back(c);
//     }
//     for(char c = 'A'; c < 'Z'; c++) {
//       str.push_back(c);
//     }
//     for(char c = '0'; c < '9'; c++) {
//       str.push_back(c);
//     }
//     return str;
//   }
//   static std::string& random_char_pool() {
//     static std::string data = __random_char_pool();
//     return data;
//   }
//   static std::string random_str(int len) {
//     static auto& char_pool = random_char_pool();
//     static std::default_random_engine rng(std::random_device{}());
//     static std::uniform_int_distribution<> dist(0, char_pool.size() - 1);
//     std::string res;
//     res.resize(len);
//     for(int i = 0; i < len; i ++) {
//       res[i] = char_pool[dist(rng)];
//     }
//     return res;
//   }
// };
template<class Session = nlohmann::json>
struct request_router {

  using session_context       = pixiu::server_bits::session::context<Session>;
  using request               = typename session_context::request;
  static constexpr auto session_id_key = request::session_id_key;

  template<class... Args>
  using handler               = std::function<response(const session_context&, Args... args)>;

  using head_handler          = std::function<void(response&, const session_context&)>;
  using get_handler           = handler<>;
  using post_handler          = handler<>;

  template<class... Args>
  using err_handler          = handler<const error::base&, Args...>;
  template<class... Args>
  using serv_err_handler     = handler<Args...>;

  using request_handler      = std::tuple<
    std::regex, head_handler, get_handler, post_handler
  >;
  using target_request_mapper = std::vector<request_handler>;
  using target_request_index  = std::map<std::string, std::size_t>;

  static auto& logger() {
    return pixiu::logger::get("router");
  }
  static response generic_error_response(const session_context& sn, const error::base& err) {
    return err.create_response(sn.req);
  }
  request_router() {
    on_err_unknown_method_      = generic_error_response;
    on_err_illegal_target_      = generic_error_response;
    on_err_target_not_found_    = generic_error_response;
    on_err_server_error_        = [](const session_context& sn, std::string what) {
      error::server_error e(what);
      return e.create_response(sn.req);
    };
  }
  template<int method_id, class Handler>
  void add_handler(const std::string& target_pattern, Handler&& h) {
    auto iter = pattern_index_.find(target_pattern);
    if(iter == pattern_index_.end()) {
      pattern_index_[target_pattern] = on_target_requests_.size();
      request_handler handler;
      std::get<0>(handler) = std::regex(target_pattern.c_str());
      std::get<method_id + 1>(handler) = std::move(h);
      on_target_requests_.emplace_back(std::move(handler));
    } else {
      std::get<method_id + 1>(on_target_requests_[iter->second]) = std::move(h);
    }
  }
  void head(const std::string& target_pattern, head_handler&& hh) {
    add_handler<0>(target_pattern, std::move(hh));
  }
  void get(const std::string& target_pattern, get_handler&& gh) {
    add_handler<1>(target_pattern, std::move(gh));
  }
  void post(const std::string& target_pattern, post_handler&& ph) {
    add_handler<2>(target_pattern, std::move(ph));
  }
  template<class... Type, class Func>
  void get(
    const std::string&    target, 
    params<Type...>&&     param_types, 
    Func&&                func
  ) {
    get(target, [p_t = std::move(param_types), func = std::move(func)](const auto& session) -> response {
      auto params_tuple = p_t.parse_get_url(session.req.target());
      return boost::hana::unpack(params_tuple, [&session, &func](auto&&... args){
        return func(session, std::move(args)...);
      });
    });
  }
  template<class... Type, class Func>
  void post(
    const std::string&    target, 
    params<Type...>&&     param_types, 
    Func&&                func
  ) {
    post(target, [p_t = std::move(param_types), func = std::move(func)](const auto& session) -> response {
      std::string req_str = session.req.body();
      logger().info("before parse: {}", req_str);
      auto params_tuple = p_t.parse_post_body(req_str);
      return boost::hana::unpack(params_tuple, [&session, &func](auto&&... args){
        return func(session, std::move(args)...);
      });
    });
  }
  void on_err_unknown_method(err_handler<>&& eh) {
    on_err_unknown_method_ = eh;
  }
  void on_err_target_not_found(err_handler<>&& eh) {
    on_err_target_not_found_ = eh;
  }
  void on_err_server_error(serv_err_handler<std::string>&& eh) {
    on_err_server_error_ = eh;
  }
  void on_err_illegal_target(err_handler<>&& eh) {
    on_err_illegal_target_ = eh;
  }
  auto search_handler(
    const std::string_view&   target, 
    std::vector<std::string>& url_capts
  ) const {
    std::smatch m_url_capt;
    std::string s_target{ target.begin(), target.end() };
    for(auto&& [pattern, head, get, post] : on_target_requests_) {
      // logger().debug("search pattern: {}", pattern.str());
      if(std::regex_match(s_target, m_url_capt, pattern)) {
        for(std::size_t i = 1; i < m_url_capt.size(); i ++) {
          url_capts.push_back(m_url_capt[i].str());
        }
        return std::make_tuple(head, get, post);
      }
    }
    throw error::target_not_found(s_target);
  }
  template<class Func>
  auto operator()(
    session_context&        session, 
    Func&&                  send
  ) const {
    auto& req = session.req;
    try {
      // Request path must be absolute and not contain "..".
      auto target = req.target();
      if( target.empty() ||
          target[0] != '/' ||
          target.find("..") != std::string_view::npos
      ) throw error::illegal_target(
        std::string{ target.begin(), target.end() }
      );
      // TODO: session process


      logger().debug("request target: {}", std::string{ target.begin(), target.end() });
      auto query_start = target.find_first_of('?');
      auto target_without_query = target.substr(0, query_start);
      auto handlers = search_handler(target_without_query, session.url_capt);

      session.session();
      // Respond to HEAD request
      switch(req.method()) {
        case __http::verb::head: 
          return method_case_head(
            session, 
            std::move(send),
            handlers
          );
        case __http::verb::get:
          return method_case_general<1>(
            session, 
            std::move(send),
            handlers
          );
        case __http::verb::post:
          return method_case_general<2>(
            session, 
            std::move(send),
            handlers
          );
        default: {
          auto method = req.method_string();
          throw error::unknown_method(std::string{method.begin(), method.end()});

        }
      };
    } catch (const error::illegal_target& err) {
      on_err_illegal_target_(session, err).write(send);
    } catch (const error::target_not_found& err) {
      on_err_target_not_found_(session, err).write(send);
    } catch (const error::unknown_method& err) {
      on_err_unknown_method_(session, err).write(send);
    } catch (const std::exception& e) {
      on_err_server_error_(session, e.what()).write(send);
    }
  }
private:
  // static constexpr int verb_to_method_id(__http::verb method) {
  //   switch(method) {
  //     case __http::verb::head:    return 0;
  //     case __http::verb::get:     return 1;
  //     case __http::verb::post:    return 2;
  //     case __http::verb::put:     return 3;
  //     case __http::verb::delete_: return 4;
  //     case __http::verb::patch:   return 5;
  //     default:                    return 6;
  //   }
  // }
  void generic_header_config(response& rep, const session_context& sn) const {
    auto& req = sn.req;
    rep.apply([&req, &sn](auto&& inter_rep){
      inter_rep.version(req.version());
      if(inter_rep.result() == __http::status::unknown) {
        inter_rep.result(__http::status::ok);
      }
      auto itr = inter_rep.find(__http::field::server);
      if(itr == inter_rep.end()) {
        inter_rep.set(__http::field::server, BOOST_BEAST_VERSION_STRING);
      }
      inter_rep.set(__http::field::set_cookie, 
        fmt::format("{}={}", session_id_key, sn.sid)
      );
      if(inter_rep.result() == __http::status::found) return;
      inter_rep.keep_alive(req.keep_alive());
    });
  }
  template<class Func, class HandlerTuple>
  auto method_case_head(const session_context& sn, Func&& send, HandlerTuple& handlers) const {
    logger().debug("head request");
    auto [head_h, get_h, post_h] = handlers;
    response rep;
    rep.emplace<__http::response<__http::empty_body>>();
    generic_header_config(rep, sn);
    head_h(rep, sn);
    return rep.write(send);
  }
  template<int method_id, class Func, class HandlerTuple>
  auto method_case_general(const session_context& sn, Func&& send, HandlerTuple& handlers) const {
    logger().debug("get request");
    auto [head_h, get_h, post_h] = handlers;
    auto rep = std::get<method_id>(handlers)(sn);
    generic_header_config(rep, sn);
    if(head_h) {
      head_h(rep, sn);
    } else {
      rep.apply([](auto& inter_rep){
        if(!inter_rep.payload_size()) {
          inter_rep.prepare_payload();
        }
        auto itr = inter_rep.find(__http::field::content_type);
        if(itr == inter_rep.end()) {
          inter_rep.set(__http::field::content_type, "text/html");
        }
      });
    }
    return rep.write(send);
  }
  err_handler<>                 on_err_unknown_method_      ;
  err_handler<>                 on_err_illegal_target_      ;
  err_handler<>                 on_err_target_not_found_    ;
  serv_err_handler<std::string> on_err_server_error_        ;
  target_request_mapper         on_target_requests_         ;
  target_request_index          pattern_index_              ;
};

}