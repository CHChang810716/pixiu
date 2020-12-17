cmake_minimum_required(VERSION 3.10.0)
include(cmake/HunterGate.cmake)
set(
    HUNTER_CACHE_SERVERS
    "https://github.com/CHChang810716/hunter-binary-cache"
    CACHE
    STRING
    "Hunter cache servers"
)
HunterGate(
    URL "https://github.com/CHChang810716/hunter.git"
    SHA1 7abc2d1c61fd32a20f3bfea7be89c924d7d0f45f
    FILEPATH ${CMAKE_CURRENT_LIST_DIR}/cmake/hunter_config.cmake
)

project(pixiu
    LANGUAGES C CXX 
    VERSION 0.0.4.0
)

hunter_add_package(Arkitekto)
find_package(Arkitekto CONFIG REQUIRED)

option(BUILD_TEST       "Build project with test"       OFF)
option(INSTALL_DEPS     "Install dependensies"          OFF)
option(BUILD_EXAMPLE    "Build project with example"    OFF)
akt_show_var(CMAKE_PREFIX_PATH)
include(cmake/toolchain_conf.cmake)
include(cmake/packages.cmake)
include(cmake/src.cmake)
include(cmake/ide.cmake)
include(cmake/install.cmake)
