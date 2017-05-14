cmake_minimum_required(VERSION 3.8)
project(ocjs)

set(CMAKE_CXX_STANDARD 11)

# Compiler options
add_compile_options(-O2 -Wall -fstrict-aliasing)

# Libraries
find_package(Java REQUIRED)
find_package(JNI REQUIRED)
include(UseJava)

# JNI
set(CLASSES_DIR ../build/classes/main/)
set(GEN_DIR "gen/")
set(NATIVE_JAVA_CLASSES com.pwootage.oc.js.DuktapeEngine)
set(JNI_GEN_HEADERS gen/com_pwootage_oc_js_DuktapeEngine.h)

add_custom_command(
        OUTPUT ${JNI_GEN_HEADERS}
        COMMAND ${Java_JAVAH_EXECUTABLE} -verbose
        -classpath ${CLASSES_DIR}
        -d ${GEN_DIR}
        -jni ${NATIVE_JAVA_CLASSES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Includes
include_directories(${JNI_INCLUDE_DIRS} ${GEN_DIR})

# Source
set(SOURCE_FILES
        ${JNI_GEN_HEADERS}
        duk_config.h
        duktape.h
        duktape.c
        DuktapeEngine.cpp)

# Build
add_library(ocjs SHARED ${SOURCE_FILES})
target_link_libraries(ocjs ${JNI_LIBRARIES})
