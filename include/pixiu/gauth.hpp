#pragma once
#include <pixiu/client.hpp>
#include <pixiu/params.hpp>
#include <pixiu/response.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <pixiu/logger.hpp>
#include <pixiu/error_code_throw.hpp>

namespace pixiu {
struct gauth_gconfig {
  const std::string     client_id;
  const std::string     auth_uri;
  const std::string     token_uri;
  const std::string     client_secret;
  static auto params() {
    return pixiu::params<opt_str>("code");
  }
};
template<class IOContextAR>
struct gauth_core {
  gauth_core(
    IOContextAR&          ioc,
    const gauth_gconfig&  config,
    const std::string&    redirect_uri,
    const std::string&    scope
  ) 
  : io_context_     (ioc              ) 
  , greq_           (make_client(ioc) ) 
  , redirect_uri_   (redirect_uri     ) 
  , response_type_  ("code"           )
  , scope_          (scope            ) 
  , gconfig_        (config           )
  {}
  template<class Ctx>
  auto callback(
    const Ctx&          ctx, 
    const opt_str&      code, 
    const std::string&  verified_redirect = "/"
  ) {
    namespace http = boost::beast::http;
    logger().debug("-> gauth callback");
    if(!code) {
      logger().debug("code not found, redirect to google");
      auto auth_uri = fmt::format(
        "{}?"
        "response_type={}&"
        "client_id={}&"
        "redirect_uri={}&"
        "scope={}",
        gconfig_.auth_uri,
        response_type_,
        gconfig_.client_id,
        redirect_uri_,
        scope_
      );
      if(access_type) {
        auth_uri += fmt::format("&access_type={}", access_type.value());
      }
      if(state) {
        auth_uri += fmt::format("&state={}", state.value());
      }
      if(include_granted_scopes) {
        auth_uri += fmt::format("&include_granted_scopes={}", include_granted_scopes.value());
      }
      if(login_hint) {
        auth_uri += fmt::format("&login_hint={}", login_hint.value());
      }
      if(prompt) {
        auth_uri += fmt::format("&prompt={}", prompt.value());
      }
      return make_redirect(auth_uri);
    } else {
      logger().debug("code found, send token request to google");
      boost::system::error_code ec;
      auto reps = greq_.async_read(
        "oauth2.googleapis.com", "80", 11, {
          {"/token", http::verb::post, {
            {"code",          code.value()            },
            {"client_id",     gconfig_.client_id      },
            {"client_secret", gconfig_.client_secret  },
            {"redirect_uri",  redirect_uri_           },
            {"grant_type",    "authorization_code"    }
          }}
        },
        ctx.get_yield_ctx()[ec]
      );
      logger().debug("token request finish, check error");
      error_code_throw(ec);
      logger().debug("no error found, save result to session");
      logger().debug("google token response: {}", msg_to_string(reps.at(0)));
      auto gauth_str = boost::beast::buffers_to_string(reps.at(0).body().data());
      logger().debug("gauth_str: {}", gauth_str);
      auto gauth_js = nlohmann::json::parse(gauth_str);
      ctx.session().google_auth = gauth_js;
      return make_redirect(verified_redirect);
    }
  }
  static auto& logger() {
    return pixiu::logger::get("gauth");
  }
  opt_str access_type               ;
  opt_str state                     ;
  opt_str include_granted_scopes    ;
  opt_str login_hint                ;
  opt_str prompt                    ;
private:
  IOContextAR&                  io_context_     ;
  pixiu::client<IOContextAR&>   greq_           ;
  const std::string             redirect_uri_   ;
  const std::string             response_type_  ;
  const std::string             scope_          ;
  const gauth_gconfig&          gconfig_        ;
  
};

constexpr struct {
  template<class IOContextAR>
  auto operator()(
    IOContextAR&                    ioc, 
    const gauth_gconfig&            config,
    const std::string&              redirect_uri,
    const std::vector<std::string>& scopes
  ) const {
    if(scopes.empty()) throw std::runtime_error("google oauth error, need scopes");
    std::string scope_all = scopes[0];
    for(std::size_t i = 1; i < scopes.size(); i ++) {
      scope_all += fmt::format(" {}", scopes[i]);
    }
    gauth_core<IOContextAR> core(
      ioc, 
      config,
      redirect_uri,
      scope_all
    );
    return core;
  }
} make_gauth;
}