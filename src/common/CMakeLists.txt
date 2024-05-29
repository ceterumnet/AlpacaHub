cmake_minimum_required(VERSION 3.14)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

file(GLOB all_SOURCES "*.cpp" "*.c")
file(GLOB all_HEADERS "*.hpp" "*.h")

add_library(common STATIC ${all_SOURCES})
target_link_libraries(common spdlog)
target_include_directories(common PUBLIC ${CMAKE_CURRENT_LIST_DIR} ${asio_INCLUDE_DIRS})
