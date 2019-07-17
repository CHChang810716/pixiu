include(cmake/scheme/lib.cmake)
target_link_libraries(${AKT_TARGET} PUBLIC 
  Boost::filesystem
)