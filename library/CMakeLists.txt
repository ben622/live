cmake_minimum_required(VERSION 3.4.1)
#支持neon汇编
ENABLE_LANGUAGE(ASM)

# 工程名称
project("Live")

#CMake预定义变量
message("当前CMake文件所在目录：${CMAKE_CURRENT_SOURCE_DIR}")
message("当前工程所在目录：${CMAKE_SOURCE_DIR}")
message("当前CMake文件的完整路径：${CMAKE_CURRENT_LIST_FILE}")
message("当前工程的路径：${PROJECT_SOURCE_DIR}")

#指定头文件
set(ROOT ${CMAKE_SOURCE_DIR}/src/main/jni)
set(PRELIB_ROOT ${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI})
INCLUDE_DIRECTORIES(${ROOT}/include)

#添加faac预编译库
ADD_LIBRARY(faac
        SHARED
        IMPORTED)
#指定静态库文件路径
SET_TARGET_PROPERTIES(faac
        PROPERTIES
        IMPORTED_LOCATION
        ${PRELIB_ROOT}/libfaac.so)

#添加x264预编译库
ADD_LIBRARY(x264
        SHARED
        IMPORTED)
#指定共享库路径
SET_TARGET_PROPERTIES(x264
        PROPERTIES
        IMPORTED_LOCATION
        ${PRELIB_ROOT}/libx264.so)

#添加rtmp模块
ADD_LIBRARY(rtmp
        SHARED
        IMPORTED)
SET_TARGET_PROPERTIES(rtmp
        PROPERTIES
        IMPORTED_LOCATION
        ${PRELIB_ROOT}/librtmp.so)

#添加libyuv
ADD_LIBRARY(yuv
        SHARED
        IMPORTED)
SET_TARGET_PROPERTIES(yuv
        PROPERTIES
        IMPORTED_LOCATION
        ${PRELIB_ROOT}/libyuv.so)

#添加opencv
ADD_LIBRARY(opencv
        SHARED
        IMPORTED)
SET_TARGET_PROPERTIES(opencv
        PROPERTIES
        IMPORTED_LOCATION
        ${PRELIB_ROOT}/libopencv_java3.so)

ADD_LIBRARY(live
        SHARED
        ${ROOT}/include/queue.c
        ${ROOT}/include/jni/JniHelpers.cpp
        ${ROOT}/include/jni/ByteArray.cpp
        ${ROOT}/include/jni/ClassRegistry.cpp
        ${ROOT}/include/jni/JavaClass.cpp
        ${ROOT}/include/jni/JavaClassUtils.cpp
        ${ROOT}/include/jni/JavaExceptionUtils.cpp
        ${ROOT}/include/jni/JavaString.cpp
        ${ROOT}/include/jni/JavaStringArray.cpp
        ${ROOT}/include/jni/JavaThreadUtils.cpp
        ${ROOT}/include/jni/NativeObject.cpp
        ${ROOT}/include/jni/ShortArray.cpp
        ${ROOT}/live.c
        ${ROOT}/native_configure.cpp
        ${ROOT}/native_chatroom_info.cpp
        ${ROOT}/native_video_push.cpp
        ${ROOT}/native_audio_push.cpp
        ${ROOT}/native_push_service.cpp
        ${ROOT}/main.cpp)


#添加NDK API
FIND_LIBRARY(log-lib
        log)

#链接库
TARGET_LINK_LIBRARIES(live
        x264
        rtmp
        faac
        yuv
        opencv
        ${log-lib})



