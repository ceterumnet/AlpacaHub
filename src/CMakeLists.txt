cmake_minimum_required(VERSION 3.24)

project(AlpacaHub CXX)

include( FetchContent )

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(3rd_Party_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/3rd_party")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

file(GLOB all_HEADERS "*.hpp" "*.h")

if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
  message(STATUS "detected 64 bit build")
  set(IS_64 true)
else()
  message(FATAL_ERROR "This does not appear to be a 64-bit platform. 32-bit not supported at this time")
endif()

set(RESTINIO_ASIO_SOURCE standalone)
set(RESTINIO_DEP_LLHTTP system)
set(RESTINIO_LLHTTP_LIB_LINK_NAME llhttp_static)
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "" FORCE)
set(BUILD_STATIC_LIBS ON CACHE INTERNAL "" FORCE)
set(RESTINIO_DEP_FMT find)
set(RESTINIO_DEP_CATCH2 find)

FetchContent_Declare(sobjectizer_src
  URL https://github.com/Stiffstream/sobjectizer/archive/v.5.8.1.1.tar.gz
)

FetchContent_Declare(fmt_src
  GIT_REPOSITORY https://github.com/fmtlib/fmt
  GIT_TAG        10.2.1
)

FetchContent_Declare(asio_src
  GIT_REPOSITORY https://github.com/chriskohlhoff/asio/
  GIT_TAG        asio-1-30-2
)

FetchContent_MakeAvailable(asio_src)

set(asio_INCLUDE_DIRS "$<BUILD_INTERFACE:${asio_src_SOURCE_DIR}/asio/include>")

message(STATUS "asio_src_BUILD_DIR: ${asio_src_BUILD_DIR}")

if(NOT asio_src_POPULATED)
  message(STATUS "asio properties not set")
endif()

FetchContent_Declare(restinio_src
  GIT_REPOSITORY https://github.com/Stiffstream/restinio
  GIT_TAG        v.0.7.2
  SOURCE_SUBDIR dev/restinio
)
FetchContent_GetProperties(restinio_src)

if(NOT restinio_src_POPULATED)
  message(STATUS "restinio properties not set")
endif()

message(STATUS "restinio_src_SOURCE_DIR: ${restinio_src_SOURCE_DIR}" )
message(STATUS "restinio_src_BUILD_DIR ${restinio_src_BUILD_DIR}")

FetchContent_Declare(llhttp_src
  URL https://github.com/nodejs/llhttp/archive/refs/tags/release/v9.1.3.tar.gz
  OVERRIDE_FIND_PACKAGE
)

# SPD Log related:
# I want to use this once gcc has
# included <format> which I believe is true in gcc v13 but raspberry
# ships with v12
# set(SPDLOG_USE_STD_FORMAT ON)

set(SPDLOG_FMT_EXTERNAL ON)
FetchContent_Declare(spdlog_src
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG        v1.14.1
)
FetchContent_Declare(json_src
  GIT_REPOSITORY https://github.com/nlohmann/json
  GIT_TAG        v3.11.3
)

FetchContent_Declare(catch2_src
  GIT_REPOSITORY https://github.com/catchorg/Catch2/
  GIT_TAG        v3.6.0
)

FetchContent_Declare(expected_lite_src
  URL https://github.com/martinmoene/expected-lite/archive/refs/tags/v0.6.3.tar.gz
)
FetchContent_MakeAvailable(fmt_src)
FetchContent_MakeAvailable(spdlog_src)

FetchContent_MakeAvailable(expected_lite_src asio_src restinio_src
  catch2_src json_src llhttp_src sobjectizer_src )

# Including date header library:
set(USE_SYSTEM_TZ_DB ON)
set(BUILD_TZ_LIB ON)
FetchContent_Declare( date_src
  GIT_REPOSITORY https://github.com/HowardHinnant/date.git
  GIT_TAG        v3.0.1
)
FetchContent_MakeAvailable(date_src)

message(STATUS "Date src: ${date_src_SOURCE_DIR}")

add_library(tz_lib ${date_src_SOURCE_DIR}/src/tz.cpp)
target_link_libraries(tz_lib PUBLIC date::date)

add_subdirectory(common)
add_subdirectory(drivers)
add_subdirectory(server)

add_executable(AlpacaHub main.cpp)

target_include_directories(AlpacaHub
  PUBLIC ${date_src_SOURCE_DIR}/include ${llhttp_src_SOURCE_DIR}/include)

target_link_libraries(AlpacaHub
  PUBLIC fmt::fmt restinio::restinio
  qhyccd spdlog::spdlog nlohmann_json::nlohmann_json
  uuid Catch2::Catch2 drivers common server llhttp_static
  PRIVATE date::date tz_lib curl)

add_executable(AlpacaHubTests
  drivers/zwo_am5_telescope.cpp
  tests/zwo_am5_commands_tests.cpp
  tests/zwo_am5_mount_tests.cpp
  tests/pegasus_alpaca_focuser_tests.cpp
)

target_link_libraries(AlpacaHubTests
  PRIVATE fmt::fmt Catch2::Catch2WithMain common server drivers
  restinio::restinio qhyccd spdlog::spdlog nlohmann_json::nlohmann_json
  uuid date::date tz_lib curl llhttp_static)

target_include_directories(AlpacaHubTests
  PUBLIC ${date_src_SOURCE_DIR}/include ${llhttp_src_SOURCE_DIR}/include)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/html DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )
