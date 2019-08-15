hunter_add_package(Arkitekto)
find_package(Arkitekto CONFIG REQUIRED)
if(BUILD_TEST)
    hunter_add_package(GTest)
    find_package(GTest CONFIG REQUIRED)
endif()
hunter_add_package(Beast)
find_package(Beast CONFIG REQUIRED)

hunter_add_package(range-v3)
find_package(range-v3 CONFIG REQUIRED)

hunter_add_package(spdlog)
find_package(spdlog CONFIG REQUIRED)

hunter_add_package(fmt)
find_package(fmt CONFIG REQUIRED)

hunter_add_package(nlohmann_json)
find_package(nlohmann_json CONFIG REQUIRED)

hunter_add_package(Boost COMPONENTS 
    filesystem
    coroutine
    context
    thread
)
find_package(Boost 
    COMPONENTS 
        filesystem
        coroutine
        context
        thread
    CONFIG
    REQUIRED
)
find_package(OpenSSL REQUIRED)
get_property(tmp_include_dirs
  TARGET OpenSSL::SSL
  PROPERTY INTERFACE_INCLUDE_DIRECTORIES
)
foreach(tmp_i ${tmp_include_dirs})
    list(APPEND AKT_VSCODE_C_CPP_INCLUDES "\"${tmp_i}\"")
endforeach()
unset(tmp_include_dirs)


find_package(Threads REQUIRED)