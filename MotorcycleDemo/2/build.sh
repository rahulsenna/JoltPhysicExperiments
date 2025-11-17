#!/bin/bash

# Configuration
JOLT_ROOT="$HOME/Developer/JoltPhysics"
JOLT_LIB="$JOLT_ROOT/Build/XCode_MacOS/libJolt.a"
BUILD_DIR="build"
OUTPUT="$BUILD_DIR/motorcycle"

# Compiler flags
CXX="clang++"
CXXFLAGS="-std=c++17 -arch arm64 -g -O0"
DEFINES="-DJPH_OBJECT_STREAM -D_DEBUG -DGL_SILENCE_DEPRECATION"
INCLUDES="-I$JOLT_ROOT -I/opt/homebrew/include"
WARNINGS="-Wno-all"

# Linker flags
LDFLAGS="-L/opt/homebrew/lib"
LIBS="-lglfw -framework Foundation -framework OpenGL -framework Cocoa -framework IOKit"

# Create build directory
mkdir -p $BUILD_DIR

# Build
echo "Building $OUTPUT..."
$CXX $CXXFLAGS $DEFINES $INCLUDES $WARNINGS \
    motorcycle.cpp \
    $JOLT_LIB \
    $LDFLAGS $LIBS \
    -o $OUTPUT

if [ $? -eq 0 ]; then
    echo "✓ Build successful: ./$OUTPUT"
else
    echo "✗ Build failed"
    exit 1
fi
