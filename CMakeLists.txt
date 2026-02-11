cmake_minimum_required(VERSION 3.16)
project(projectVI_webserver)

set(CMAKE_CXX_STANDARD 17)

add_executable(server src/main.cpp)

target_include_directories(server PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party/asio/include
)

target_link_libraries(server PRIVATE odbc32)