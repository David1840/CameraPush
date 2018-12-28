
cmake_minimum_required(VERSION 3.4.1)

add_library(
        ffmpeg-handler
        SHARED
        src/main/jni/ffmpeg-handler.c)

set(JNI_LIBS_DIR ${CMAKE_SOURCE_DIR}/src/main/jniLibs)

find_library( # Sets the name of the path variable.
        log-lib
        log)
add_library(avutil
        SHARED
        IMPORTED )
set_target_properties(avutil
        PROPERTIES IMPORTED_LOCATION
        ${JNI_LIBS_DIR}/${ANDROID_ABI}/libavutil-56.so )

add_library(swresample
        SHARED
        IMPORTED )
set_target_properties(swresample
        PROPERTIES IMPORTED_LOCATION
        ${JNI_LIBS_DIR}/${ANDROID_ABI}/libswresample-3.so )

add_library(swscale
        SHARED
        IMPORTED )
set_target_properties(swscale
        PROPERTIES IMPORTED_LOCATION
        ${JNI_LIBS_DIR}/${ANDROID_ABI}/libswscale-5.so )

add_library(avcodec
        SHARED
        IMPORTED )
set_target_properties(avcodec
        PROPERTIES IMPORTED_LOCATION
        ${JNI_LIBS_DIR}/${ANDROID_ABI}/libavcodec-58.so )

add_library(avformat
        SHARED
        IMPORTED )
set_target_properties(avformat
        PROPERTIES IMPORTED_LOCATION
        ${JNI_LIBS_DIR}/${ANDROID_ABI}/libavformat-58.so )

add_library(avfilter
        SHARED
        IMPORTED )
set_target_properties(avfilter
        PROPERTIES IMPORTED_LOCATION
        ${JNI_LIBS_DIR}/${ANDROID_ABI}/libavfilter-7.so )

add_library(avdevice
        SHARED
        IMPORTED )
set_target_properties(avdevice
        PROPERTIES IMPORTED_LOCATION
        ${JNI_LIBS_DIR}/${ANDROID_ABI}/libavdevice-58.so )

include_directories(${JNI_LIBS_DIR}/include)

target_link_libraries(ffmpeg-handler
        avutil swresample swscale avcodec avformat avfilter avdevice
        ${log-lib} ${android-lib} )