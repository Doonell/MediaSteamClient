cmake_minimum_required(VERSION 3.16)

project(ZHMedia LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(3RDPARTY_DIR "${CMAKE_SOURCE_DIR}/3rdparty")
set(FFMPEG_DIR "${CMAKE_SOURCE_DIR}/3rdparty/ffmpeg")

set(FFMPEG_LIB_DIR "${FFMPEG_DIR}/ffmpeg-4.3.1-2020-11-19-full_build-shared/lib")
set(FFMPEG_INCLUDE_DIR "${FFMPEG_DIR}/ffmpeg-4.3.1-2020-11-19-full_build-shared/include")
set(SDL_INCLUDE_DIR "${3RDPARTY_DIR}/SDL2/include")
set(SDL_LIB_DIR "${3RDPARTY_DIR}/SDL2/lib/x64")
set(LIBRTMP_LIB_DIR "${3RDPARTY_DIR}/librtmp")

file(GLOB SOURCES "*.cpp")

# 添加 librtmp 子目录
add_subdirectory(${LIBRTMP_LIB_DIR})

add_executable(ZHMedia ${SOURCES})

include(GNUInstallDirs)
install(TARGETS ZHMedia
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

link_directories(
    ${FFMPEG_LIB_DIR}
    ${SDL_LIB_DIR}
)
include_directories(
    ${FFMPEG_INCLUDE_DIR}
    ${SDL_INCLUDE_DIR}
)

# 链接库文件
target_link_libraries(ZHMedia PRIVATE
    rtmp
    "${FFMPEG_LIB_DIR}/avutil.lib"
    "${FFMPEG_LIB_DIR}/avformat.lib"
    "${FFMPEG_LIB_DIR}/avcodec.lib"
    "${FFMPEG_LIB_DIR}/avdevice.lib"
    "${FFMPEG_LIB_DIR}/avfilter.lib"
    "${FFMPEG_LIB_DIR}/avutil.lib"
    "${FFMPEG_LIB_DIR}/postproc.lib"
    "${FFMPEG_LIB_DIR}/swresample.lib"
    "${FFMPEG_LIB_DIR}/swscale.lib"
    "${SDL_LIB_DIR}/SDL2.lib"          # 添加 SDL2 主库
    "${SDL_LIB_DIR}/SDL2main.lib"      # 添加 SDL2main 库
)
