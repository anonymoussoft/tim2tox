#!/bin/bash
# Smart build script for libtim2tox_ffi.dylib
# Only rebuilds if library doesn't exist or source files are newer

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
FFI_BUILD_DIR="$BUILD_DIR/ffi"
LIB_FILE="$FFI_BUILD_DIR/libtim2tox_ffi.dylib"
FFI_SOURCE_DIR="$SCRIPT_DIR/ffi"

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}Checking if libtim2tox_ffi.dylib needs to be built...${NC}"

# Check if library exists
if [[ ! -f "$LIB_FILE" ]]; then
    echo -e "${YELLOW}Library not found. Building...${NC}"
    NEEDS_BUILD=true
else
    # Check if any source files are newer than the library
    NEEDS_BUILD=false
    
    # Get library modification time
    LIB_TIME=$(stat -f "%m" "$LIB_FILE" 2>/dev/null || stat -c "%Y" "$LIB_FILE" 2>/dev/null)
    
    # Check FFI source files
    for file in "$FFI_SOURCE_DIR"/*.cpp "$FFI_SOURCE_DIR"/*.h "$FFI_SOURCE_DIR"/*.hpp; do
        if [[ -f "$file" ]]; then
            FILE_TIME=$(stat -f "%m" "$file" 2>/dev/null || stat -c "%Y" "$file" 2>/dev/null)
            if [[ $FILE_TIME -gt $LIB_TIME ]]; then
                echo -e "${YELLOW}Source file $file is newer than library. Rebuilding...${NC}"
                NEEDS_BUILD=true
                break
            fi
        fi
    done
    
    # Check CMakeLists.txt
    if [[ -f "$FFI_SOURCE_DIR/CMakeLists.txt" ]]; then
        CMAKE_TIME=$(stat -f "%m" "$FFI_SOURCE_DIR/CMakeLists.txt" 2>/dev/null || stat -c "%Y" "$FFI_SOURCE_DIR/CMakeLists.txt" 2>/dev/null)
        if [[ $CMAKE_TIME -gt $LIB_TIME ]]; then
            echo -e "${YELLOW}CMakeLists.txt is newer than library. Rebuilding...${NC}"
            NEEDS_BUILD=true
        fi
    fi
    
    # Check if build directory is configured
    if [[ ! -f "$FFI_BUILD_DIR/CMakeCache.txt" ]]; then
        echo -e "${YELLOW}Build directory not configured. Rebuilding...${NC}"
        NEEDS_BUILD=true
    fi
fi

if [[ "$NEEDS_BUILD" == "true" ]]; then
    echo -e "${BLUE}Building libtim2tox_ffi.dylib...${NC}"
    
    # Ensure build directory exists
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    # Set up PKG_CONFIG_PATH and CMAKE_PREFIX_PATH to help CMake find dependencies
    # Check if Homebrew is installed and set paths accordingly
    if command -v brew >/dev/null 2>&1; then
        BREW_PREFIX=$(brew --prefix)
        export PKG_CONFIG_PATH="${BREW_PREFIX}/opt/opus/lib/pkgconfig:${BREW_PREFIX}/opt/libvpx/lib/pkgconfig:${BREW_PREFIX}/opt/libconfig/lib/pkgconfig:${PKG_CONFIG_PATH:-}"
        export CMAKE_PREFIX_PATH="${BREW_PREFIX}/opt/opus:${BREW_PREFIX}/opt/libvpx:${BREW_PREFIX}/opt/libconfig:${CMAKE_PREFIX_PATH:-}"
    fi
    
    # Configure CMake if needed
    # Check if required options are enabled in cache, if not, reconfigure
    if [[ ! -f "CMakeCache.txt" ]] || ! grep -q "BUILD_TOXAV:BOOL=ON" "CMakeCache.txt" 2>/dev/null || \
       ! grep -q "MUST_BUILD_TOXAV:BOOL=ON" "CMakeCache.txt" 2>/dev/null || \
       ! grep -q "DHT_BOOTSTRAP:BOOL=ON" "CMakeCache.txt" 2>/dev/null || \
       ! grep -q "BOOTSTRAP_DAEMON:BOOL=ON" "CMakeCache.txt" 2>/dev/null; then
        if [[ ! -f "CMakeCache.txt" ]]; then
            echo -e "${BLUE}Configuring CMake...${NC}"
        else
            echo -e "${YELLOW}Required build options not enabled in cache, reconfiguring...${NC}"
        fi
        cmake .. \
            -DCMAKE_BUILD_TYPE=Release \
            -DSTRICT_ABI=OFF \
            -DBOOTSTRAP_DAEMON=ON \
            -DBUILD_TOXAV=ON \
            -DMUST_BUILD_TOXAV=ON \
            -DDHT_BOOTSTRAP=ON \
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
            -DDEBUG=OFF \
            -DBUILD_FFI=ON
    fi
    
    # Build only the FFI library
    echo -e "${BLUE}Building tim2tox_ffi target...${NC}"
    make -j$(sysctl -n hw.ncpu 2>/dev/null || echo 4) tim2tox_ffi
    
    if [[ -f "$LIB_FILE" ]]; then
        echo -e "${GREEN}✅ Successfully built libtim2tox_ffi.dylib${NC}"
        
        # Verify Dart_PostCObject_DL symbol
        if nm -g "$LIB_FILE" 2>/dev/null | grep -q "Dart_PostCObject_DL"; then
            echo -e "${GREEN}✅ Verified: Dart_PostCObject_DL symbol found${NC}"
        else
            echo -e "${YELLOW}⚠️  Warning: Dart_PostCObject_DL symbol not found${NC}"
        fi
    else
        echo -e "${YELLOW}⚠️  Warning: Library file not found after build${NC}"
        exit 1
    fi
else
    echo -e "${GREEN}✅ Library is up to date. Skipping build.${NC}"
fi

cd "$SCRIPT_DIR"
