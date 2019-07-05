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

hunter_add_package(Boost COMPONENTS filesystem)
find_package(Boost 
    COMPONENTS 
        filesystem
    CONFIG
    REQUIRED
)

find_package(Threads REQUIRED)