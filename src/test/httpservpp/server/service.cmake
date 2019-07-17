include(cmake/scheme/test.cmake)
target_link_libraries(${AKT_TARGET} PRIVATE httpservpp-server-service)