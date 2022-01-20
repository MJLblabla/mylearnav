# Sets the minimum version of CMake required to build the native library.
cmake_minimum_required(VERSION 3.4.1)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")

set(jnilibs ${CMAKE_SOURCE_DIR}/../jniLibs)
set(libname ffplay)

include_directories(
        include
        glm
        common
        ${CMAKE_SOURCE_DIR}/util
        ${CMAKE_SOURCE_DIR}/common
        ${CMAKE_SOURCE_DIR}/play
        ${CMAKE_SOURCE_DIR}/play/decoder
        ${CMAKE_SOURCE_DIR}/play/render
        ${CMAKE_SOURCE_DIR}/play/render/audio
        ${CMAKE_SOURCE_DIR}/play/render/video)

link_directories(
        ${jnilibs}/${ANDROID_ABI})

file(GLOB src-files
        ${CMAKE_SOURCE_DIR}/*.cpp
        ${CMAKE_SOURCE_DIR}/util/*.cpp
        ${CMAKE_SOURCE_DIR}/common/*.cpp
        ${CMAKE_SOURCE_DIR}/play/*.cpp
        ${CMAKE_SOURCE_DIR}/play/decoder/*.cpp
        ${CMAKE_SOURCE_DIR}/play/render/video/*.cpp
        ${CMAKE_SOURCE_DIR}/play/render/audio/*.cpp
        )

add_library( # Sets the name of the library.
        ${libname}

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        ${src-files}
        )


set(third-party-libs
        avformat
        avcodec
        avfilter
        swresample
        swscale
        avutil
        fdk-aac
        x264
        )

set(native-libs
        android
        mediandk
        EGL
        GLESv3
        OpenSLES
        log
        m
        z
        )

target_link_libraries( # Specifies the target library.
        ${libname}

        # Links the target library to the log library
        # included in the NDK.
        ${log-lib}
        ${third-party-libs}
        ${native-libs}
        )