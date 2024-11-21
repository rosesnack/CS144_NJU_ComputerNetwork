cmake_minimum_required(VERSION 3.24.2)

project(minnow CXX)

if(${CMAKE_GENERATOR} STREQUAL "Unix Makefiles")
  set(CMAKE_MAKE_PROGRAM "${PROJECT_SOURCE_DIR}/scripts/make-parallel.sh" CACHE STRING "" FORCE)
endif()

if(${PROJECT_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "Minnow must be built outside its source directory, e.g. `cmake -B build`.")
endif()

include(etc/build_type.cmake)
include(etc/cflags.cmake)
include(etc/scanners.cmake)
include(etc/tests.cmake)

include_directories("${PROJECT_SOURCE_DIR}/util")
include_directories("${PROJECT_SOURCE_DIR}/src")

add_subdirectory("${PROJECT_SOURCE_DIR}/util")
add_subdirectory("${PROJECT_SOURCE_DIR}/src")
add_subdirectory("${PROJECT_SOURCE_DIR}/tests")
add_subdirectory("${PROJECT_SOURCE_DIR}/apps")

# 确保线程库被正确链接
find_package(Threads REQUIRED)

# 链接线程库到特定目标
target_link_libraries(tcp_ipv4 Threads::Threads)
target_link_libraries(webget Threads::Threads)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
