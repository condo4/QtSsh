message(STATUS "QtSsh subdir: ${CMAKE_CURRENT_LIST_DIR}")
add_subdirectory(${CMAKE_CURRENT_LIST_DIR})
include_directories(${CMAKE_CURRENT_LIST_DIR}/qtssh)
