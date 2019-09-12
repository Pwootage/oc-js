cmake_minimum_required(VERSION 3.8)
project(ocjs)

set(CMAKE_CXX_STANDARD 17)

# Compiler options
#add_compile_options(-O2 -Wall -fstrict-aliasing -stdlib=libc++)
add_compile_options(-O2 -g -Wall -fstrict-aliasing -stdlib=libc++)
#add_compile_options(-O0 -g -Wall -fstrict-aliasing -stdlib=libc++)
#add_definitions(-DDEBUG)
add_definitions(-DJS_GENERATE_JVM_CONVERT)

# Libraries
find_package(Java REQUIRED)
find_package(JNI REQUIRED)
include(UseJava)

# JNI
set(CLASSES_DIR ../build/classes/kotlin/main/)
set(GEN_DIR "gen/")
set(NATIVE_JAVA_CLASSES
  com.pwootage.oc.js.duktape.DuktapeEngine
  com.pwootage.oc.js.duktape.DuktapeStatic
  com.pwootage.oc.js.spidermonkey.SpiderMonkeyEngine
  com.pwootage.oc.js.spidermonkey.SpiderMonkeyStatic)
set(JNI_GEN_HEADERS
  gen/com_pwootage_oc_js_duktape_DuktapeEngine.h
  gen/com_pwootage_oc_js_duktape_DuktapeStatic.h
  gen/com_pwootage_oc_js_spidermonkey_SpiderMonkeyEngine.h
  gen/com_pwootage_oc_js_spidermonkey_SpiderMonkeyStatic.h)

add_custom_command(
  OUTPUT ${JNI_GEN_HEADERS}
  COMMAND ${Java_JAVAH_EXECUTABLE} -verbose
  -classpath ${CLASSES_DIR}
  -d ${GEN_DIR}
  -jni ${NATIVE_JAVA_CLASSES}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Includes
include_directories(SYSTEM ${JNI_INCLUDE_DIRS} ${GEN_DIR} include)

# Source
set(SOURCE_FILES
  ${JNI_GEN_HEADERS}
  duktape.cpp
  SpiderMonkeyEngine.cpp
  SpiderMonkeyEngineNative.cpp
  SpiderMonkeyEngineNative.h
  DuktapeEngine.cpp
  DuktapeEngineNative.cpp
  DuktapeEngineNative.h JSValue.cpp JSValue.hpp)

# imported libs
add_library(mozjs SHARED IMPORTED)
#add_library(mozglue SHARED IMPORTED)

# Build
add_library(ocjs SHARED ${SOURCE_FILES})
target_link_libraries(ocjs ${JNI_LIBRARIES} mozjs)

add_executable(ocjs_test test_main.cpp)
target_link_libraries(ocjs_test ocjs)

if (UNIX AND NOT APPLE)
  set(LINUX TRUE)
endif ()

if (WIN32)

elseif (APPLE)
  set_target_properties(mozjs PROPERTIES IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/libmozjs-69.dylib)
  #  set_target_properties(mozglue PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/spidermonkey/macos.x64/libmozglue.dylib)
  add_custom_command(TARGET ocjs
    POST_BUILD COMMAND
    ${CMAKE_INSTALL_NAME_TOOL} -add_rpath "@executable_path/../../Frameworks/"
    $<TARGET_FILE:ocjs>)
  target_link_libraries(ocjs)
else ()
  target_link_libraries(ocjs
    z
    rt
    pthread
    -stdlib=libc++)
endif ()
