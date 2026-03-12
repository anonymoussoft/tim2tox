#!/bin/bash

# Exit on error
set -e

echo "Building tim2tox examples..."

# Create build directory
mkdir -p build
cd build

# Configure CMake
cmake .. \
    -DCMAKE_BUILD_TYPE=Release

# Build
make -j$(sysctl -n hw.ncpu)

echo "Build completed successfully!"
echo ""
echo "To run the examples:"
echo "1. Start the echo bot server:"
echo "   ./build/echo_bot_server"
echo ""
echo "2. In another terminal, run the client:"
echo "   ./build/tim2tox_client <server_tox_id>"
echo ""
echo "Note: Copy the Tox ID from the server output to use with the client." 
echo ""
echo "To run the automated echo test:"
echo "   ./test_echo.sh"