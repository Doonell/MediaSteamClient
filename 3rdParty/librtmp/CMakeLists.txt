cmake_minimum_required(VERSION 3.16)

# 设置库名称
project(librtmp LANGUAGES C)

# 查找当前目录下的所有源文件
file(GLOB LIBRTMP_SOURCES
    "*.c"
    "*.h"
)

# 添加静态库 librtmp.a
add_library(rtmp STATIC ${LIBRTMP_SOURCES})

# 设置头文件路径
target_include_directories(rtmp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})