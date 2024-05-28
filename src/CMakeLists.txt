cmake_minimum_required(VERSION 3.14)

project(AlpacaHub CXX)

set(CMAKE_CXX_STANDARD 17)

set(3rd_Party_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/3rd_party")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

file(GLOB all_SOURCES "*.cpp" "*.c")
file(GLOB all_HEADERS "*.hpp" "*.h")

# file(GLOB driver_SOURCES "drivers/*.cpp" "drivers/*.c")
# file(GLOB driver_HEADERS "drivers/*.hpp" "drivers/*.h")

if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
  message(STATUS "detected 64 bit build")
  set(IS_64 true)
else()
  message(FATAL_ERROR "This does not appear to be a 64-bit platform. 32-bit not supported at this time")
endif()

set(restinio_ROOT "${3rd_Party_ROOT}/restinio-0.7.2/dev")

set(RESTINIO_ASIO_SOURCE standalone)
set(RESTINIO_DEP_STANDALONE_ASIO find)

set(RESTINIO_DEP_LLHTTP local)

set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "" FORCE)
set(BUILD_STATIC_LIBS ON CACHE INTERNAL "" FORCE)

# add_subdirectory("${3rd_Party_ROOT}/fmt-10.2.1")

# SPD Log related:
# I want to use this once gcc has
# included <format> which I believe is true in gcc v13 but raspberry
# ships with v12
# set(SPDLOG_USE_STD_FORMAT ON)

set(asio_INCLUDE_DIRS "${restinio_ROOT}/asio/include")
add_subdirectory("${restinio_ROOT}/fmt")
add_subdirectory("${restinio_ROOT}/nodejs/llhttp")
add_subdirectory("${restinio_ROOT}/expected-lite")
add_subdirectory("${restinio_ROOT}/restinio")

set(SPDLOG_FMT_EXTERNAL ON)
add_subdirectory("${3rd_Party_ROOT}/spdlog-1.14.1")

add_subdirectory("${3rd_Party_ROOT}/json-3.11.3")

add_subdirectory("${3rd_Party_ROOT}/Catch2-3.6.0")

add_subdirectory(common)
add_subdirectory(drivers)
add_subdirectory(server)

add_executable(AlpacaHub ${all_SOURCES})

# target_include_directories(AlpacaHub ${driver_HEADERS})

message(STATUS "asio_INCLUDE_DIRS: ${asio_INCLUDE_DIRS}")

target_link_libraries(AlpacaHub PUBLIC restinio qhyccd spdlog nlohmann_json uuid Catch2 drivers common server)

# add_executable(AlpacaHubTests )
# target_link_libraries(AlpacaHubTests PRIVATE Catch2::Catch2WithMain restinio qhyccd spdlog nlohmann_json uuid)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/html DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )
