#pragma once
#include <boost/asio/ssl/stream.hpp>
inline void load_server_certificate(boost::asio::ssl::context& ctx) {
  /*
      The certificate was generated from CMD.EXE on Windows 10 using:

      winpty openssl dhparam -out dh.pem 2048
      winpty openssl req -newkey rsa:2048 -nodes -keyout key.pem -x509 -days 10000 -out cert.pem -subj "//C=US\ST=CA\L=Los Angeles\O=Beast\CN=www.example.com"
  */

  std::string const cert =
    "-----BEGIN CERTIFICATE-----\n"
    "-----END CERTIFICATE-----\n"
  ;

  std::string const key =
    "-----BEGIN PRIVATE KEY-----\n"
    "-----END PRIVATE KEY-----\n"
  ;

  std::string const dh = 
    "-----BEGIN DH PARAMETERS-----\n"
    "-----END DH PARAMETERS-----\n"
  ;

  ctx.set_password_callback(
    [](std::size_t,
        boost::asio::ssl::context_base::password_purpose)
    {
        return "test";
    });

  ctx.set_options(
      boost::asio::ssl::context::default_workarounds |
      boost::asio::ssl::context::no_sslv2 |
      boost::asio::ssl::context::single_dh_use);
  // ctx.set_options(
  //     boost::asio::ssl::context::default_workarounds |
  //     boost::asio::ssl::context::no_sslv2);

  ctx.use_certificate_chain(
    boost::asio::buffer(cert.data(), cert.size()));

  ctx.use_private_key(
    boost::asio::buffer(key.data(), key.size()),
    boost::asio::ssl::context::file_format::pem);
  ctx.use_tmp_dh(
    boost::asio::buffer(dh.data(), dh.size()));
}