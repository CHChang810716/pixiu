[![Build status](https://ci.appveyor.com/api/projects/status/b4745dwte80u73je/branch/master?svg=true)](https://ci.appveyor.com/project/CHChang810716/pixiu/branch/master)
[![Build Status](https://travis-ci.org/CHChang810716/pixiu.svg?branch=master)](https://travis-ci.org/CHChang810716/pixiu)

# Intro

This is a basic and light-weight C++ http server library base on Boost.Beast and C++17 language standard.

Here is a hello world example.

## Server side

```c++
#include <pixiu/server.hpp>
#include <pixiu/response.hpp>

int main() {
  /**
   * make a http server and listen to 8080 port
   */
  auto server = pixiu::make_server();
  server.get("/", [](const auto& req) -> pixiu::server_bits::response {
    http::response<http::string_body> rep;
    rep.body() = "hello world";
    return pixiu::server_bits::response(rep);
  });
  server.listen("0.0.0.0", 8080);
  server.run();
}
```

Now use browser to access local host 8080 port, you will get "hello world".

## Convenient & type safe parameter handle

```c++
#include <pixiu/server.hpp>
#include <pixiu/response.hpp>

int main() {
  /**
   *  Define request parameter parser
   */
  pixiu::request_router router;
  router.get("/", params<int, float>("a", "b"), 
    [](const auto& req, int a, float b) {
      return pixiu::make_response(std::to_string(a + b));
    }
  );

  /**
   *  Make a http server and listen to 8080 port
   */
  auto server = pixiu::make_server(router);
  server.listen("0.0.0.0", 8080);
  server.run();
}
```

# Build Project

## Recommended environments

* g++ >= 7.3
* CMake >= 3.13
* Linux, Windows is tested (see CI tag)
* On Windows, mingw is recommended

## Steps

```bash
>cd pixiu
>mkdir build
>cd build
>cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../stage -DBUILD_TEST=ON
# build with 4 core
>cmake --build . --target install -j 4
# run test
>ctest
```

# Use Package

This project is using CMake Hunter to maintain 3rd party, but not yet register to official Hunter site.

Therefore, use Hunter submodule solution to link this package is recommended, see [here](https://hunter.readthedocs.io/en/latest/user-guides/hunter-user/git-submodule.html) for more details.
