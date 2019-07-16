target_include_directories(${AKT_TARGET} PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)
target_include_directories(${AKT_TARGET} PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(${AKT_TARGET} PRIVATE Threads::Threads)
if(CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(${AKT_TARGET} PUBLIC -ftemplate-backtrace-limit=0)
endif()
if(UNIX)
    target_link_libraries(${AKT_TARGET} PRIVATE ${CMAKE_DL_LIBS} rt)
endif()
if(MINGW)
    target_compile_options(${AKT_TARGET} PUBLIC -Wa,-mbig-obj)
endif()