cmake_minimum_required(VERSION 3.8)
project(ffmpeg4.0)


set(CMAKE_INCLUDE_DIRECTORIES_BEFORE ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -v -stdlib=libc++ \
-framework OpenGL \
-framework AppKit \
-framework Security \
-framework CoreFoundation \
-framework CoreVideo \
-framework CoreMedia \
-framework QuartzCore \
-framework CoreFoundation \
-framework VideoDecodeAcceleration \
-framework Cocoa \
-framework AudioToolbox \
-framework VideoToolbox \
-framework OpenCL \
-I/usr/local/include \
-L/usr/local/lib/ \
-I/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include \
-L/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/lib/")

find_package(PkgConfig REQUIRED)
pkg_check_modules(ASS REQUIRED libass libpng x264 x265 opus liblzma fdk-aac vpx lame sdl gsm)

include_directories(
        ${CMAKE_SOURCE_DIR}/../../ffmpeg-4.0.2/build/include
)


set(CMAKE_CXX_STANDARD 11)

set(FFMPEG_LIBS
        ${CMAKE_SOURCE_DIR}/../../ffmpeg-4.0.2/build/lib/libavcodec.a
        ${CMAKE_SOURCE_DIR}/../../ffmpeg-4.0.2/build/lib/libavdevice.a
        ${CMAKE_SOURCE_DIR}/../../ffmpeg-4.0.2/build/lib/libavfilter.a
        ${CMAKE_SOURCE_DIR}/../../ffmpeg-4.0.2/build/lib/libavformat.a
        ${CMAKE_SOURCE_DIR}/../../ffmpeg-4.0.2/build/lib/libavutil.a
        ${CMAKE_SOURCE_DIR}/../../ffmpeg-4.0.2/build/lib/libpostproc.a
        ${CMAKE_SOURCE_DIR}/../../ffmpeg-4.0.2/build/lib/libswresample.a
        ${CMAKE_SOURCE_DIR}/../../ffmpeg-4.0.2/build/lib/libswscale.a
        ${ASS_LIBRARIES}
        /usr/lib/libbz2.dylib
        /usr/lib/libiconv.dylib
        /usr/lib/libssl.dylib
        /usr/lib/libz.dylib)

add_subdirectory(version)
add_subdirectory(test)

