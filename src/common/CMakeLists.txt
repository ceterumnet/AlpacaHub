cmake_minimum_required(VERSION 3.24)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

file(GLOB all_SOURCES "*.cpp" "*.c")
file(GLOB all_HEADERS "*.hpp" "*.h")

add_library(common STATIC ${all_SOURCES} ${all_HEADERS})
target_link_libraries(common spdlog nlohmann_json::nlohmann_json)
target_include_directories(common PUBLIC ${CMAKE_CURRENT_LIST_DIR}/.. ${asio_INCLUDE_DIRS})
