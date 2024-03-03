cmake_minimum_required(VERSION 3.25)
project(MyMuduo)

#设置动态库输出路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

#设置编译参数
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11")

#头文件路径
include_directories(${PROJECT_SOURCE_DIR}/include)

#源文件
aux_source_directory(./src SRC_LIST)

#生成库
add_library(MyMuduo SHARED ${SRC_LIST})