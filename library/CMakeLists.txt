cmake_minimum_required(VERSION 3.4.1)

# 工程名称
project("Live")

#CMake预定义变量
message("当前CMake文件所在目录：${CMAKE_CURRENT_SOURCE_DIR}")
message("当前工程所在目录：${CMAKE_SOURCE_DIR}")
message("当前CMake文件的完整路径：${CMAKE_CURRENT_LIST_FILE}")
message("当前工程的路径：${PROJECT_SOURCE_DIR}")

#指定头文件
set(ROOT ${CMAKE_SOURCE_DIR}/src/main/jni)
set(PRELIB_ROOT ${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}/lib)
INCLUDE_DIRECTORIES(${ROOT}/include ${ROOT}/include/jni )

#添加faac预编译库
ADD_LIBRARY(faac
        STATIC
        IMPORTED)
#指定静态库文件路径
SET_TARGET_PROPERTIES(faac
        PROPERTIES
        IMPORTED_LOCATION
        ${PRELIB_ROOT}/libfaac.a)
#添加x264预编译库
ADD_LIBRARY(x264
        STATIC
        IMPORTED)
#指定共享库路径
SET_TARGET_PROPERTIES(x264
        PROPERTIES
        IMPORTED_LOCATION
        ${PRELIB_ROOT}/libx264.a)

#添加rtmp模块
ADD_LIBRARY(rtmp
        STATIC
        IMPORTED)
SET_TARGET_PROPERTIES(rtmp
        PROPERTIES
        IMPORTED_LOCATION
        ${PRELIB_ROOT}/librtmp.a)

ADD_LIBRARY(live
        SHARED
        ${ROOT}/include/queue.c
        ${ROOT}/live.c
        ${ROOT}/native_configure.cpp
        ${ROOT}/native_chatroom_info.cpp
        ${ROOT}/main.cpp
        ${ROOT}/native_push_service.cpp)


#添加NDK API
FIND_LIBRARY(log-lib
        log)

#链接库
TARGET_LINK_LIBRARIES(live
        x264
        rtmp
        faac
        ${log-lib})



