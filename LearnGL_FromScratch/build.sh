#!/bin/bash

# Configuration
BUILD_DIR="build"
IMGUI_ROOT="$HOME/Developer/imgui"
OUTPUT="$BUILD_DIR/main"

# Compiler flags
CXX="clang++"
CXXFLAGS="-std=c++23 -arch arm64 -Wno-error -g -O0"
DEFINES="-D_DEBUG -DGL_SILENCE_DEPRECATION"
INCLUDES="-I/opt/homebrew/include -I$IMGUI_ROOT -I$IMGUI_ROOT/backends"
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
    main.cpp graphics_api_gl.cpp shader.cpp \
    $IMGUI_SOURCES \
    $LDFLAGS $LIBS \
    -o $OUTPUT

if [ $? -eq 0 ]; then
    echo "✓ Build successful: ./$OUTPUT"
else
    echo "✗ Build failed"
    exit 1
fi
