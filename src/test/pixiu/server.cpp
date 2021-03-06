#include <pixiu/server.hpp>
#include <gtest/gtest.h>
#include <pixiu/client.hpp>
#include <pixiu/response.hpp>
#include <pixiu/request_utils.hpp>
#include <pixiu/request_router.hpp>
#include <thread>

using namespace boost::beast;

auto& logger() {
  return pixiu::logger::get("testc");
}

class server_test 
: public ::testing::Test 
{
protected:
  static void SetUpTestCase() {
    nlohmann::json data;
    auto& loggers = data["loggers"];
    loggers["http"] = {
      {"level", "debug"}
    };
    loggers["http_base"] = {
      {"level", "debug"}
    };
    loggers["core"] = {
      {"level", "debug"}
    };
    loggers["router"] = {
      {"level", "debug"}
    };
    loggers["client"] = {
      {"level", "debug"}
    };
    loggers["testc"] = {
      {"level", "debug"}
    };
    loggers["fiber"] = {
      {"level", "debug"}
    };
    pixiu::logger::config(data);
  }
};
template<class ServIOC, class Func>
auto client_run(ServIOC& serv_ioc, Func&& func) {
  std::thread t([&serv_ioc](){
    serv_ioc.run_for(std::chrono::seconds(30));
  });
  boost::asio::io_context ioc2;
  auto client = pixiu::make_client(ioc2);
  boost::asio::steady_timer serv_ready(ioc2, std::chrono::seconds(5));
  boost::asio::spawn(ioc2, [&](auto yield){
    serv_ready.async_wait(yield);
    func(client, yield);
  });
  ioc2.run();
  t.join();
}
template<class... T>
using params = pixiu::server_bits::params<T...>;
TEST(params_test, parse) {
  params<int, float, std::string, std::uint16_t> parser(
    "integer", "float32", "str", "uint_16bit"
  );
  auto tuple = parser.parse_get_url("www.asdf.com/mysite?integer=100&float32=3.414&str=qsefth&uint_16bit=65535");
  EXPECT_EQ(boost::hana::at_c<0>(tuple), int(100));
  EXPECT_TRUE(std::abs(boost::hana::at_c<1>(tuple) - float(3.414)) < 0.0001);
  EXPECT_EQ(boost::hana::at_c<2>(tuple), "qsefth");
  EXPECT_EQ(boost::hana::at_c<3>(tuple), std::uint16_t(65535));
}

TEST_F(server_test, router_call) {
  pixiu::request_router<> router;
  router.get("/", params<int, float>("a", "b"), 
    [](const auto& req, int a, float b) {
      return pixiu::make_response(std::to_string(a + b));
    }
  );
  pixiu::server_bits::session::context<> session;
  session.req = pixiu::make_request(
    http::verb::get,
    "localhost:8080","/", 
    11, {
      {"a", 12},
      {"b", 2.4}
    }
  );

  router(session, [](auto&& rep){
    using Rep = std::decay_t<decltype(rep)>;
    if constexpr(std::is_same_v<typename Rep::body_type, http::string_body>) {
      auto actual = std::stod(rep.body());
      EXPECT_TRUE(std::abs(actual - 14.4) < 0.001);
    }
  });
}
template<class Rep>
auto rep_to_string(Rep&& rep) {
  return pixiu::msg_to_string(std::forward<Rep>(rep));
}
struct session_data {
  int counter {0};
  bool primary {false};
};
TEST_F(server_test, manual_request_router) {
  std::string test_actual;

  pixiu::request_router<session_data> router;
  router.get("/", params<
    int, 
    std::optional<float>
  >("a", "b"), 
    [](const auto& ctx, int a, std::optional<float> b) {
      return pixiu::make_response(std::to_string(a + b.value_or(-0.5)));
    }
  );
  router.get("/session_id", 
    [](const auto& ctx) {
      auto& ses = ctx.session();
      return pixiu::make_response(ctx.session_id());
    }
  );
  router.get("/incr", [](const auto& ctx){
    auto& sn = ctx.session();
    return pixiu::make_response(
      std::to_string(sn.counter ++)
    );
  });
  router.get("/articles/(.+)", [](const auto& ctx){
    return pixiu::make_response(ctx.url_capt.at(0));
  });
  router.post("/login", 
    params<
      std::string, 
      std::optional<std::string>
    >("user", "passwd"),
    [](auto& ctx, 
      const std::string& user, 
      const std::optional<std::string>& passwd
    ){
      EXPECT_EQ(user, "john");
      EXPECT_EQ(passwd, "qsefth");
      return pixiu::make_response(nlohmann::json({
        {"ec", 0},
        {"msg", "login success"},
        {"token", "azscfb"}
      }));
    }
  );

  auto server = pixiu::make_server(router);
  server.listen("0.0.0.0", 8080);

  client_run(server, [&test_actual](auto& client, auto yield){
    boost::system::error_code ec;
    {
      auto reps = client.async_read(
        "localhost", "8080", 
        11, {
          {"/", http::verb::get, {
            {"a", 12},
            {"b", 2.4}
          } }
        }, 
        yield[ec]
      );
      test_actual = buffers_to_string(reps.at(0).body().data());
      EXPECT_LT(std::abs(std::stod(test_actual) - 14.4), 0.01);
    }
    {
      auto reps = client.async_read(
        "localhost", "8080", 
        11, {
          {"/session_id", http::verb::get}
        }, 
        yield
      );
      auto str = buffers_to_string(reps.at(0).body().data());
      auto set_cookie = reps.at(0)[http::field::set_cookie];
      EXPECT_GT(str.size(), 0);
      EXPECT_GT(set_cookie.size(), 0);
    }
    {
      auto reps = client.async_read(
        "localhost", "8080", 
        11, {
          {"/session_id", http::verb::get}
        }, 
        [](auto&& req) {
          req.set(http::field::cookie, "pixiu_session_id=qsefthuk90");
        },
        yield 
      );
      auto set_cookie = reps.at(0)[boost::beast::http::field::set_cookie];
      EXPECT_NE(set_cookie, "qsefthuk90");
    }
    {
      std::string cookie_str;
      std::vector<http::response<http::dynamic_body>> reps;
      for(int i = 0; i < 5; i++) {
        if(i == 0) {
          reps = client.async_read(
            "localhost", "8080", 
            11, {
              {"/incr", http::verb::get}
            },
            yield 
          );
        } else {
          reps = client.async_read(
            "localhost", "8080", 
            11, {
              {"/incr", http::verb::get}
            }, 
            [&](auto&& req) {
              req.set(http::field::cookie, cookie_str);
            },
            yield 
          );
        }
        cookie_str = reps.at(0)[http::field::set_cookie];
        auto str = buffers_to_string(reps.at(0).body().data());
        EXPECT_EQ(str, std::to_string(i));
      }
    }
    {
      auto reps = client.async_read(
        "localhost", "8080", 
        11, {
          {"/articles/startabc", http::verb::get}
        }, 
        yield
      );
      auto str = buffers_to_string(reps.at(0).body().data());
      EXPECT_EQ(str, "startabc");
    }
    {
      auto reps = client.async_read(
        "localhost", "8080", 
        11, {
          {"/login", http::verb::post}
        }, 
        [&](auto&& req) {
          nlohmann::json form;
          form["user"] = "john";
          form["passwd"] = "qsefth";
          auto str = form.dump();
          logger().info(str);
          req.body() = str;
          req.set(http::field::content_type, "application/json");
          req.set(http::field::content_length, str.length());
        }, 
        yield
      );
      auto str = buffers_to_string(reps.at(0).body().data());
      logger().info(str);
      // EXPECT_EQ(str, "startabc");
    }
  });
}