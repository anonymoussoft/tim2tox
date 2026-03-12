cmake_minimum_required(VERSION 3.4.1)

# Add nlohmann/json library (header-only, recommended for better JSON parsing)
# Option 1: Using FetchContent (automatically downloads at configure time)
include(FetchContent)
FetchContent_Declare(json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
)
set(JSON_Install OFF CACHE INTERNAL "")
set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(json)

# FFI shared library for Flutter/Dart bindings
# Note: dart_api_dl.c will be added after we find it
add_library(tim2tox_ffi SHARED 
    tim2tox_ffi.cpp
    callback_bridge.cpp
    json_parser.cpp
    dart_compat_layer.cpp
    dart_compat_utils.cpp
    dart_compat_listeners.cpp
    dart_compat_callbacks.cpp
    dart_compat_sdk.cpp
    dart_compat_message.cpp
    dart_compat_friendship.cpp
    dart_compat_conversation.cpp
    dart_compat_group.cpp
    dart_compat_user.cpp
    dart_compat_signaling.cpp
    dart_compat_community.cpp
    dart_compat_other.cpp
)

# Set include directories
target_include_directories(tim2tox_ffi PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../source
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/c-toxcore
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/c-toxcore/toxcore
)

# Add Dart API DL include path
# Try to find Flutter/Dart SDK include directory
if(DEFINED ENV{FLUTTER_ROOT})
    set(FLUTTER_ROOT $ENV{FLUTTER_ROOT})
else()
    # Try to find Flutter from PATH
    find_program(FLUTTER_BIN flutter)
    if(FLUTTER_BIN)
        # Get the directory containing the flutter binary (bin/)
        get_filename_component(FLUTTER_BIN_DIR ${FLUTTER_BIN} DIRECTORY)
        # Get the parent directory (Flutter root)
        get_filename_component(FLUTTER_ROOT ${FLUTTER_BIN_DIR} DIRECTORY)
    endif()
endif()

# Try Flutter root paths
set(DART_SDK_PATHS)
if(FLUTTER_ROOT)
    list(APPEND DART_SDK_PATHS "${FLUTTER_ROOT}/bin/cache/dart-sdk/include")
endif()
# Add fallback paths
list(APPEND DART_SDK_PATHS
    "$ENV{HOME}/flutter/bin/cache/dart-sdk/include"
    "/usr/local/flutter/bin/cache/dart-sdk/include"
    "/opt/flutter/bin/cache/dart-sdk/include"
)

# Find and add Dart API DL include path and source file
set(DART_API_DL_C_PATH "")
foreach(DART_PATH IN LISTS DART_SDK_PATHS)
    if(EXISTS "${DART_PATH}/dart_api_dl.h")
        target_include_directories(tim2tox_ffi PRIVATE ${DART_PATH})
        message(STATUS "Found Dart API DL at: ${DART_PATH}")
        # Also find dart_api_dl.c source file
        if(EXISTS "${DART_PATH}/dart_api_dl.c")
            set(DART_API_DL_C_PATH "${DART_PATH}/dart_api_dl.c")
            message(STATUS "Found Dart API DL source at: ${DART_API_DL_C_PATH}")
            # Add dart_api_dl.c to the library sources
            target_sources(tim2tox_ffi PRIVATE "${DART_API_DL_C_PATH}")
        else()
            message(WARNING "dart_api_dl.c not found at ${DART_PATH}, Dart API DL symbols may not be available")
        endif()
        break()
    endif()
endforeach()

# If dart_api_dl.c was not found, warn but continue
if(NOT DART_API_DL_C_PATH)
    message(WARNING "dart_api_dl.c not found in any Dart SDK path. Dart_PostCObject_DL symbols will not be available.")
endif()

# Platform-specific include directories
if(APPLE)
    # macOS - check for both Intel and Apple Silicon homebrew paths
    if(EXISTS "/opt/homebrew/include")
        target_include_directories(tim2tox_ffi PRIVATE /opt/homebrew/include)
    elseif(EXISTS "/usr/local/include")
        target_include_directories(tim2tox_ffi PRIVATE /usr/local/include)
    endif()
elseif(UNIX AND NOT APPLE)
    # Linux - use standard system paths and pkg-config
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SODIUM libsodium)
    if(SODIUM_FOUND)
        target_include_directories(tim2tox_ffi PRIVATE ${SODIUM_INCLUDE_DIRS})
    endif()
elseif(WIN32)
    # Windows - support vcpkg and standard paths
    if(DEFINED ENV{VCPKG_ROOT})
        target_include_directories(tim2tox_ffi PRIVATE $ENV{VCPKG_ROOT}/installed/x64-windows/include)
    endif()
    # Also check standard Windows include paths
    target_include_directories(tim2tox_ffi PRIVATE "C:/vcpkg/installed/x64-windows/include")
endif()

# Link dependencies
target_link_libraries(tim2tox_ffi
    tim2tox
    toxcore_static
    nlohmann_json::nlohmann_json  # Add nlohmann/json library
)

# Platform-specific library linking
if(APPLE)
    # macOS - check for both Intel and Apple Silicon homebrew paths
    if(EXISTS "/opt/homebrew/lib/libsodium.dylib")
        target_link_libraries(tim2tox_ffi /opt/homebrew/lib/libsodium.dylib)
    elseif(EXISTS "/usr/local/lib/libsodium.dylib")
        target_link_libraries(tim2tox_ffi /usr/local/lib/libsodium.dylib)
    else()
        # Try to find libsodium using find_library
        find_library(SODIUM_LIB
            NAMES sodium libsodium
            PATHS
                /opt/homebrew/lib
                /usr/local/lib
                /usr/lib
        )
        if(SODIUM_LIB)
            target_link_libraries(tim2tox_ffi ${SODIUM_LIB})
        else()
            message(WARNING "libsodium not found, linking may fail")
        endif()
    endif()
elseif(UNIX AND NOT APPLE)
    # Linux - use pkg-config to find libsodium
    if(SODIUM_FOUND)
        target_link_libraries(tim2tox_ffi ${SODIUM_LIBRARIES})
    else()
        # Fallback to find_library
        find_library(SODIUM_LIB
            NAMES sodium libsodium
            PATHS
                /usr/lib
                /usr/local/lib
                /lib
        )
        if(SODIUM_LIB)
            target_link_libraries(tim2tox_ffi ${SODIUM_LIB})
        else()
            message(WARNING "libsodium not found, linking may fail")
        endif()
    endif()
elseif(WIN32)
    # Windows - support vcpkg and standard paths
    if(DEFINED ENV{VCPKG_ROOT})
        find_library(SODIUM_LIB
            NAMES libsodium sodium
            PATHS
                $ENV{VCPKG_ROOT}/installed/x64-windows/lib
                $ENV{VCPKG_ROOT}/installed/x64-windows/debug/lib
        )
        if(SODIUM_LIB)
            target_link_libraries(tim2tox_ffi ${SODIUM_LIB})
        endif()
    endif()
    # Also check standard Windows library paths
    find_library(SODIUM_LIB_WIN
        NAMES libsodium sodium
        PATHS
            C:/vcpkg/installed/x64-windows/lib
            C:/vcpkg/installed/x64-windows/debug/lib
    )
    if(SODIUM_LIB_WIN)
        target_link_libraries(tim2tox_ffi ${SODIUM_LIB_WIN})
    endif()
endif()

# Link toxav dependencies if BUILD_TOXAV is enabled
# Note: toxav code is compiled into toxcore_static, so we don't need to link toxav_static
# But we need to link OPUS and VPX libraries that toxav depends on
if(BUILD_TOXAV)
    # Define preprocessor macro for conditional compilation
    target_compile_definitions(tim2tox_ffi PRIVATE BUILD_TOXAV=1)
    
    # Link OPUS and VPX libraries
    if(TARGET Opus::opus AND TARGET libvpx::libvpx)
        target_link_libraries(tim2tox_ffi Opus::opus libvpx::libvpx)
    elseif(TARGET PkgConfig::OPUS AND TARGET PkgConfig::VPX)
        target_link_libraries(tim2tox_ffi PkgConfig::OPUS PkgConfig::VPX)
    else()
        # Fallback: use pkg-config variables
        if(OPUS_FOUND)
            target_link_libraries(tim2tox_ffi ${OPUS_LIBRARIES})
            target_include_directories(tim2tox_ffi PRIVATE ${OPUS_INCLUDE_DIRS})
        endif()
        if(VPX_FOUND)
            target_link_libraries(tim2tox_ffi ${VPX_LIBRARIES})
            target_include_directories(tim2tox_ffi PRIVATE ${VPX_INCLUDE_DIRS})
        endif()
    endif()
endif()

# Platform-specific linker options
if(APPLE)
    # macOS: Use undefined dynamic lookup for Dart API DL functions
    # These functions are provided by the Dart VM at runtime
    set_target_properties(tim2tox_ffi PROPERTIES
        LINK_FLAGS "-undefined dynamic_lookup"
    )
endif()

# Set library properties
set_target_properties(tim2tox_ffi PROPERTIES
    OUTPUT_NAME "tim2tox_ffi"
    POSITION_INDEPENDENT_CODE ON
)

# Install rules
install(TARGETS tim2tox_ffi
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install FFI header
install(FILES tim2tox_ffi.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tim2tox
)

