#!/bin/bash

# Configuration
JOLT_ROOT="$HOME/Developer/JoltPhysics"
JOLT_LIB="$JOLT_ROOT/Build/XCode_MacOS/libJolt.a"
BUILD_DIR="build"
IMGUI_ROOT="$HOME/Developer/imgui"
OUTPUT="$BUILD_DIR/game.dylib"

# Compiler flags
CXX="clang++"
CXXFLAGS="-std=c++23 -arch arm64 -Wno-error -dynamiclib  -undefined dynamic_lookup -fPIC -O0 -g -O0"
DEFINES="-DJPH_OBJECT_STREAM -D_DEBUG -DGL_SILENCE_DEPRECATION"
INCLUDES="-I$JOLT_ROOT -I/opt/homebrew/include -I$IMGUI_ROOT -I$IMGUI_ROOT/backends"
WARNINGS="-Wno-all"

# Linker flags
LDFLAGS="-L/opt/homebrew/lib"
LIBS="-lglfw -framework Foundation -framework OpenGL -framework Cocoa -framework IOKit"

# Create build directory
mkdir -p $BUILD_DIR


# ImGui source files
IMGUI_SOURCES="$IMGUI_ROOT/imgui.cpp \
               $IMGUI_ROOT/imgui_demo.cpp \
               $IMGUI_ROOT/imgui_draw.cpp \
               $IMGUI_ROOT/imgui_tables.cpp \
               $IMGUI_ROOT/imgui_widgets.cpp \
               $IMGUI_ROOT/backends/imgui_impl_glfw.cpp \
               $IMGUI_ROOT/backends/imgui_impl_opengl3.cpp"


# Build
echo "Building $OUTPUT..."
$CXX $CXXFLAGS $DEFINES $INCLUDES $WARNINGS \
    game.cpp \
    $IMGUI_SOURCES \
    $JOLT_LIB \
    $LDFLAGS $LIBS \
    -o $OUTPUT

if [ $? -eq 0 ]; then
    echo "✓ Build successful: ./$OUTPUT"
else
    echo "✗ Build failed"
    exit 1
fi
