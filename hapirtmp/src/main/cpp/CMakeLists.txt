# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.10.2)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
# Declares and names the project.
set(jnilibs ${CMAKE_SOURCE_DIR}/../jniLibs)
project("rtmpliving")
set(libname rtmpliving)

include_directories(
        include
        include/libyuv
        include/librtmp
        recorder
        hwcodec
        common
        util
)

link_directories(
        ${jnilibs}/${ANDROID_ABI})

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.


file(GLOB src-files
        ${CMAKE_SOURCE_DIR}/*.cpp
        ${CMAKE_SOURCE_DIR}/recorder/*.cpp
        ${CMAKE_SOURCE_DIR}/hwcodec/*.cpp
        )

add_library( # Sets the name of the library.
        ${libname}

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        ${src-files}
        )


set(third-party-libs
        rtmp
        yuv
        )

set(native-libs
        android
        mediandk
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