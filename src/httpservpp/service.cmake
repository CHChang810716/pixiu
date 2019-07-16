include(cmake/scheme/lib.cmake)
target_link_libraries(${AKT_TARGET} PUBLIC 
  httpservpp-logger
  OpenSSL::SSL
  OpenSSL::Crypto
)
if(MINGW)
  target_link_libraries(${AKT_TARGET} PUBLIC 
    ws2_32
    wsock32
  )
endif()