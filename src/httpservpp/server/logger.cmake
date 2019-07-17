include(cmake/scheme/lib.cmake)
target_link_libraries(${AKT_TARGET} PUBLIC 
  spdlog::spdlog
  httpservpp-server-path
)