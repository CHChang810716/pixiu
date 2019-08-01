include(cmake/scheme/lib.cmake)
target_link_libraries(${AKT_TARGET} PUBLIC 
  pixiu-logger
  OpenSSL::SSL
  OpenSSL::Crypto
  Boost::coroutine
  Boost::context
)
if(MINGW)
  target_link_libraries(${AKT_TARGET} PUBLIC 
    ws2_32
    wsock32
  )
endif()