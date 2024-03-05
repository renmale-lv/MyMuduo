cmake_minimum_required(VERSION 3.16)
project(example)

SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-pthread")

add_executable(example ./main.cpp)

target_link_libraries(example PRIVATE ${PROJECT_SOURCE_DIR}/../lib/libMyMuduo.so)
target_include_directories(example PRIVATE ${PROJECT_SOURCE_DIR}/../include)