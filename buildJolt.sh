cd ~/Developer/JoltPhysics/Build

# Clean
rm -rf XCode_MacOS

# Build Debug for arm64
cmake -S .. -B XCode_MacOS \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DTARGET_UNIT_TESTS=OFF \
    -DTARGET_HELLO_WORLD=OFF \
    -DTARGET_PERFORMANCE_TEST=OFF \
    -DTARGET_SAMPLES=OFF \
    -DTARGET_VIEWER=OFF \
    -DENABLE_OBJECT_STREAM=ON \
    -DDOUBLE_PRECISION=OFF \
    -DDEBUG_RENDERER_IN_DEBUG_AND_RELEASE=OFF \
    -DPROFILER_IN_DEBUG_AND_RELEASE=OFF

# Build
cmake --build XCode_MacOS --config Debug -j8

# Verify
lipo -info XCode_MacOS/libJolt.a


#-------------------------------------------
cd ~/Developer/JoltPhysics/Build

# Clean previous build
rm -rf XCode_MacOS

# Set Homebrew LLVM as compiler
export PATH="/opt/homebrew/opt/llvm/bin:$PATH"
export CC=/opt/homebrew/opt/llvm/bin/clang
export CXX=/opt/homebrew/opt/llvm/bin/clang++

# Build with C++23
cmake -S . -B XCode_MacOS \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DCMAKE_C_COMPILER=/opt/homebrew/opt/llvm/bin/clang \
    -DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++ \
    -DCMAKE_CXX_STANDARD=23 \
    -DTARGET_UNIT_TESTS=OFF \
    -DTARGET_HELLO_WORLD=OFF \
    -DTARGET_PERFORMANCE_TEST=OFF \
    -DTARGET_SAMPLES=OFF \
    -DTARGET_VIEWER=OFF \
    -DENABLE_OBJECT_STREAM=ON \
    -DDOUBLE_PRECISION=OFF \
    -DDEBUG_RENDERER_IN_DEBUG_AND_RELEASE=OFF \
    -DPROFILER_IN_DEBUG_AND_RELEASE=OFF

cmake --build XCode_MacOS --config Debug -j8

# Verify
file XCode_MacOS/libJolt.a


#----------------------------------------

# find brew libs
brew --prefix glfw
