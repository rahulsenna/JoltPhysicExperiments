cmake_minimum_required(VERSION 3.16)
project(MotorcycleExample)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Build architecture for macOS" FORCE)
endif()

set(JOLT_PHYSICS_ROOT "$ENV{HOME}/Developer/JoltPhysics" CACHE PATH "Path to JoltPhysics root")
set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)

find_library(JOLT_LIBRARY 
    NAMES Jolt
    PATHS "${JOLT_PHYSICS_ROOT}/Build/XCode_MacOS"
    REQUIRED
    NO_DEFAULT_PATH
)

# Find GLFW (install with: brew install glfw)
find_package(glfw3 REQUIRED)

include_directories("${JOLT_PHYSICS_ROOT}")

add_executable(motorcycle motorcycle.cpp)

target_compile_definitions(motorcycle PRIVATE 
    JPH_OBJECT_STREAM
    _DEBUG
    GL_SILENCE_DEPRECATION
)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "AppleClang")
    # target_compile_options(motorcycle PRIVATE -Wall -g -O0)
    target_compile_options(motorcycle PRIVATE -Wno-all -Wno-error -g -O0)
endif()
target_compile_options(motorcycle PRIVATE -Wno-all -Wno-error -g -O0)

# Link GLFW and OpenGL
target_link_libraries(motorcycle PRIVATE 
    ${JOLT_LIBRARY}
    glfw
)

if(APPLE)
    find_library(FOUNDATION_FRAMEWORK Foundation REQUIRED)
    find_library(OPENGL_FRAMEWORK OpenGL REQUIRED)
    find_library(COCOA_FRAMEWORK Cocoa REQUIRED)
    find_library(IOKIT_FRAMEWORK IOKit REQUIRED)
    target_link_libraries(motorcycle PRIVATE 
        ${FOUNDATION_FRAMEWORK}
        ${OPENGL_FRAMEWORK}
        ${COCOA_FRAMEWORK}
        ${IOKIT_FRAMEWORK}
    )
endif()

message(STATUS "===========================================")
message(STATUS "Architecture: ${CMAKE_OSX_ARCHITECTURES}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Jolt library: ${JOLT_LIBRARY}")
message(STATUS "===========================================")
