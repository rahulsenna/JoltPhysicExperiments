#!/bin/bash

# Configuration
JOLT_ROOT="$HOME/Developer/JoltPhysics"
JOLT_LIB="$JOLT_ROOT/Build/XCode_MacOS/libJolt.a"
BUILD_DIR="build"
OUTPUT="$BUILD_DIR/game.dylib"

# Compiler flags
CXX="clang++"
CXXFLAGS="-std=c++23 -arch arm64 -Wno-error -dynamiclib  -undefined dynamic_lookup -fPIC -g -O0"
DEFINES="-DJPH_OBJECT_STREAM -D_DEBUG -DGL_SILENCE_DEPRECATION"
INCLUDES="-I$JOLT_ROOT -I/opt/homebrew/include"
WARNINGS="-Wno-all"

# Linker flags
LDFLAGS="-L/opt/homebrew/lib"
LIBS="-lpthread"

# Create build directory
mkdir -p $BUILD_DIR


# Build
echo "Building $OUTPUT..."
$CXX $CXXFLAGS $DEFINES $INCLUDES $WARNINGS \
    game.cpp mesh.cpp \
    $JOLT_LIB \
    $LDFLAGS $LIBS \
    -o $OUTPUT

if [ $? -eq 0 ]; then
    echo "✓ Build successful: ./$OUTPUT"
else
    echo "✗ Build failed"
    exit 1
fi
