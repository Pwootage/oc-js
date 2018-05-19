cmake_minimum_required(VERSION 3.8)
project(ocjs)

set(CMAKE_CXX_STANDARD 17)

# Compiler options
#add_compile_options(-O2 -Wall -fstrict-aliasing -stdlib=libc++)
add_compile_options(-O0 -g -Wall -fstrict-aliasing -stdlib=libc++)

# Libraries
find_package(Java REQUIRED)
find_package(JNI REQUIRED)
include(UseJava)

# JNI
set(CLASSES_DIR ../build/classes/main/)
set(GEN_DIR "gen/")
set(NATIVE_JAVA_CLASSES
        com.pwootage.oc.js.v8.V8Engine
        com.pwootage.oc.js.v8.V8Static)
set(JNI_GEN_HEADERS
        gen/com_pwootage_oc_js_v8_V8Engine.h
        gen/com_pwootage_oc_js_v8_V8Static.h)

add_custom_command(
        OUTPUT ${JNI_GEN_HEADERS}
        COMMAND ${Java_JAVAH_EXECUTABLE} -verbose
        -classpath ${CLASSES_DIR}
        -d ${GEN_DIR}
        -jni ${NATIVE_JAVA_CLASSES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Includes
include_directories(${JNI_INCLUDE_DIRS} ${GEN_DIR} include)

# Source
set(SOURCE_FILES
        ${JNI_GEN_HEADERS}
        duk_config.h
        duktape.h
        #        duktape.c
        V8Engine.cpp
        V8EngineNative.cpp
        V8EngineNative.h)

# Build
add_library(ocjs SHARED ${SOURCE_FILES})
target_link_libraries(ocjs ${JNI_LIBRARIES})

add_executable(ocjs_test test_main.cpp)
target_link_libraries(ocjs_test ocjs)

if (UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif ()

if (WIN32)

elseif (APPLE)
    target_link_libraries(ocjs
            ${CMAKE_SOURCE_DIR}/v8/macos.x64/libv8_base.a
            ${CMAKE_SOURCE_DIR}/v8/macos.x64/libv8_external_snapshot.a
            ${CMAKE_SOURCE_DIR}/v8/macos.x64/libicui18n.a
            ${CMAKE_SOURCE_DIR}/v8/macos.x64/libicuuc.a
            ${CMAKE_SOURCE_DIR}/v8/macos.x64/libinspector.a
            ${CMAKE_SOURCE_DIR}/v8/macos.x64/libv8_libplatform.a
            ${CMAKE_SOURCE_DIR}/v8/macos.x64/libv8_libsampler.a
            ${CMAKE_SOURCE_DIR}/v8/macos.x64/libv8_libbase.a)
else ()
    target_link_libraries(ocjs
            ${CMAKE_SOURCE_DIR}/v8/linux.x64/libv8_base.a
            ${CMAKE_SOURCE_DIR}/v8/linux.x64/libv8_external_snapshot.a
            ${CMAKE_SOURCE_DIR}/v8/linux.x64/libicui18n.a
            ${CMAKE_SOURCE_DIR}/v8/linux.x64/libicuuc.a
            ${CMAKE_SOURCE_DIR}/v8/linux.x64/libinspector.a
            ${CMAKE_SOURCE_DIR}/v8/linux.x64/libv8_libplatform.a
            ${CMAKE_SOURCE_DIR}/v8/linux.x64/libv8_libsampler.a
            ${CMAKE_SOURCE_DIR}/v8/linux.x64/libv8_libbase.a
            z
            rt
            pthread
            -stdlib=libc++)
endif ()
