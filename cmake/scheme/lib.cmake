akt_add_library(${AKT_TARGET} lib_targets ${AKT_CHILD_SRCS})
include(${CMAKE_CURRENT_LIST_DIR}/basic_target_setting.cmake)
target_link_libraries(${AKT_TARGET} PUBLIC 
    ${AKT_CHILD_TARGETS}
)