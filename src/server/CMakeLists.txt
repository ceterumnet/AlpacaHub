cmake_minimum_required(VERSION 3.24)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

file(GLOB all_SOURCES "*.cpp" "*.c")
file(GLOB all_HEADERS "*.hpp" "*.h")

add_library(server ${all_SOURCES})
target_link_libraries(server nlohmann_json::nlohmann_json spdlog Catch2 restinio date::date)
target_include_directories(server PUBLIC ${CMAKE_CURRENT_LIST_DIR}/.. ${asio_INCLUDE_DIRS} ${llhttp_src_SOURCE_DIR}/include)
