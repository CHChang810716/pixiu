# Pixiu (貔貅)

travis [![Build Status](https://travis-ci.org/CHChang810716/pixiu.svg?branch=master)](https://travis-ci.org/CHChang810716/pixiu)
appveyor [![Build status](https://ci.appveyor.com/api/projects/status/b4745dwte80u73je/branch/master?svg=true)](https://ci.appveyor.com/project/CHChang810716/pixiu/branch/master)

Pixiu is a light weight RESTful C++ http server based on [Boost.Beast](https://github.com/boostorg/beast).

Provides protocols:

* HTTP (working/preview)
* HTTPS (pending)
* Websocket (pending )

## Install

Pixiu use [Hunter](https://github.com/ruslo/hunter) package manager to maintain the third party dependensies and of course packed as Hunter package.

### Build from source

Linux

```shell
pixiu> mkdir build
pixiu> cd build
# configure, can add -DBUILD_TEST to build the unit test
pixiu> cmake ..
# build project, can add --target install to install the build result to system path
pixiu> cmake --build .
```

Windows(MinGW)

```batch
pixiu> mkdir build
pixiu> cd build
:: Pixiu use OpenSSL to develop TLS module but Hunter provides poor support for MinGW
pixiu> cmake -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="<path to OpenSSL>" ..
pixiu> cmake --build .
```

### Get Pixiu from Hunter

\<still working\>

### Example

Shortest way to start a http hello world server

```cpp
auto server = pixiu::make_server();
server.get("/", [](const auto& req) -> pixiu::server_bits::response {
  http::response<http::string_body> rep;
  rep.body() = "hello world";
  return pixiu::server_bits::response(rep);
});
server.listen("0.0.0.0", 8080);
server.run();
```
