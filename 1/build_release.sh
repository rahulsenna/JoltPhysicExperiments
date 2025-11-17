#!/bin/bash

JOLT_ROOT="$HOME/Developer/JoltPhysics"
JOLT_LIB="$JOLT_ROOT/Build/XCode_MacOS/libJolt.a"
BUILD_DIR="build"
OUTPUT="$BUILD_DIR/motorcycle"

CXX="clang++"
CXXFLAGS="-std=c++17 -arch arm64 -O3 -DNDEBUG"
DEFINES="-DJPH_OBJECT_STREAM -DGL_SILENCE_DEPRECATION"
INCLUDES="-I$JOLT_ROOT -I/opt/homebrew/include"
WARNINGS="-Wno-all"

LDFLAGS="-L/opt/homebrew/lib"
LIBS="-lglfw -framework Foundation -framework OpenGL -framework Cocoa -framework IOKit"

# Create build directory
mkdir -p $BUILD_DIR

echo "Building $OUTPUT (Release)..."
$CXX $CXXFLAGS $DEFINES $INCLUDES $WARNINGS \
    motorcycle.cpp \
    $JOLT_LIB \
    $LDFLAGS $LIBS \
    -o $OUTPUT

if [ $? -eq 0 ]; then
    echo "✓ Build successful: ./$OUTPUT"
    strip $OUTPUT
    echo "✓ Stripped binary"
else
    echo "✗ Build failed"
    exit 1
fi
