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
if(WIN32)
    set(H_CMAKE_MODUE_PATH ${H_CMAKE_MODUE_PATH})
    unset(CMAKE_MODULE_PATH)
    find_package(OpenSSL REQUIRED)
    set(CMAKE_MODULE_PATH ${H_CMAKE_MODUE_PATH})
else()
    hunter_add_package(OpenSSL)
    find_package(OpenSSL REQUIRED)
endif()
get_property(tmp_include_dirs
  TARGET OpenSSL::SSL
  PROPERTY INTERFACE_INCLUDE_DIRECTORIES
)
foreach(tmp_i ${tmp_include_dirs})
    list(APPEND AKT_VSCODE_C_CPP_INCLUDES "\"${tmp_i}\"")
endforeach()
unset(tmp_include_dirs)
akt_show_var(OPENSSL_LIBRARIES)
foreach(tmp_lib ${OPENSSL_LIBRARIES})
    get_filename_component(tmp_lib_dir "${tmp_lib}" DIRECTORY)
    get_filename_component(tmp_lib_inst_dir "${tmp_lib_dir}" DIRECTORY)
    list(APPEND BUNDLE_RT_DIRS ${tmp_lib_dir})
    list(APPEND BUNDLE_RT_DIRS ${tmp_lib_inst_dir}/bin)
    unset(tmp_lib_dir)
    unset(tmp_lib)
endforeach()
akt_show_var(BUNDLE_RT_DIRS)


find_package(Threads REQUIRED)