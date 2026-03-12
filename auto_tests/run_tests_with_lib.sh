#!/bin/bash
# Run tests with native library path configured
# Automatically builds library if needed

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
TIM2TOX_DIR="$PROJECT_ROOT/tim2tox"
LIB_SOURCE="$TIM2TOX_DIR/build/ffi/libtim2tox_ffi.dylib"

# Build library if needed (smart build - only rebuilds if necessary)
if [[ -f "$TIM2TOX_DIR/build_ffi.sh" ]]; then
    echo "Checking if library needs to be built..."
    "$TIM2TOX_DIR/build_ffi.sh"
else
    echo "Warning: build_ffi.sh not found. Skipping automatic build."
fi

# Find Flutter executable directory
FLUTTER_EXE=$(which flutter)
FLUTTER_BIN_DIR=$(dirname "$FLUTTER_EXE")
FLUTTER_ENGINE_DIR="$FLUTTER_BIN_DIR/cache/artifacts/engine/darwin-x64"

# Create symlink in Flutter engine directory (where dlopen searches)
if [[ -f "$LIB_SOURCE" && -d "$FLUTTER_ENGINE_DIR" ]]; then
  # Create symlink
  ln -sf "$LIB_SOURCE" "$FLUTTER_ENGINE_DIR/libtim2tox_ffi.dylib" 2>/dev/null || true
  echo "Created symlink: $FLUTTER_ENGINE_DIR/libtim2tox_ffi.dylib -> $LIB_SOURCE"
else
  echo "Warning: Library not found at $LIB_SOURCE or Flutter engine dir not found"
fi

# Change to test directory
cd "$SCRIPT_DIR"

# Run tests
flutter test --no-pub "$@"

# Note: We don't remove the symlink as it might be used by other tests
