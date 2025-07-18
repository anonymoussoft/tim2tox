#!/bin/bash

# Exit on error
set -e

# Create build directory
mkdir -p build
cd build

# Configure CMake
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DSTRICT_ABI=OFF \
    -DBOOTSTRAP_DAEMON=OFF \
    -DBUILD_TOXAV=OFF \
    -DMUST_BUILD_TOXAV=OFF \
    -DDHT_BOOTSTRAP=OFF \
    -DENABLE_SHARED=OFF \
    -DENABLE_STATIC=ON \
    -DUNITTEST=OFF \
    -DAUTOTEST=OFF \
    -DBUILD_MISC_TESTS=OFF \
    -DBUILD_FUN_UTILS=OFF \
    -DBUILD_FUZZ_TESTS=OFF \
    -DUSE_IPV6=ON \
    -DEXPERIMENTAL_API=OFF \
    -DERROR=ON \
    -DWARNING=ON \
    -DINFO=ON \
    -DTRACE=OFF \
    -DDEBUG=OFF

# Build using available CPU cores
make -j$(sysctl -n hw.ncpu)

# Return to original directory
cd .. 