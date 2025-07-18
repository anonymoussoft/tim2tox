cmake_minimum_required(VERSION 3.4.1)

project(tim2tox VERSION 0.1.0 LANGUAGES C CXX)

# Set C/C++ standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build settings
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Optional dependencies
option(BUILD_TOXAV "Whether to build the tox AV library" OFF)
option(MUST_BUILD_TOXAV "Fail the build if toxav cannot be built" OFF)
option(DHT_BOOTSTRAP "Enable building of DHT_bootstrap" OFF)
option(BOOTSTRAP_DAEMON "Enable building of tox-bootstrapd" OFF)
option(BUILD_FUZZ_TESTS "Build fuzzing harnesses" OFF)

# Feature options
option(USE_IPV6 "Use IPv6" ON)
option(EXPERIMENTAL_API "Enable experimental API features" OFF)
option(TRACE "Enable TRACE level logging" OFF)
option(DEBUG "Enable DEBUG level logging" OFF)
option(ERROR "Enable ERROR level logging" ON)
option(WARNING "Enable WARNING level logging" ON)
option(INFO "Enable INFO level logging" ON)

# Build options
option(FLAT_OUTPUT_STRUCTURE "Whether to produce output artifacts in bin/lib" OFF)
option(BUILD_MISC_TESTS "Build additional tests" OFF)
option(BUILD_FUN_UTILS "Build additional just for fun utilities" OFF)
option(UNITTEST "Enable unit tests" OFF)
option(AUTOTEST "Enable autotests (mainly for CI)" OFF)
option(NON_HERMETIC_TESTS "Whether to build and run tests that depend on an internet connection" OFF)
option(PROXY_TEST "Enable proxy test" OFF)

# Platform specific options
if(MSVC)
    option(MSVC_STATIC_SODIUM "Whether to link libsodium statically for MSVC" OFF)
endif()

# Library type options
option(ENABLE_SHARED "Build shared libraries" OFF)
option(ENABLE_STATIC "Build static libraries" ON)

# Additional build options
option(ENABLE_LOGGING "Enable logging support" ON)
option(ENABLE_TESTS "Build test suite" OFF)
option(ENABLE_EXAMPLES "Build example applications" OFF)
option(USE_SANITIZER "Enable sanitizer support (address, thread, undefined)" OFF)

# Set output directories
if(FLAT_OUTPUT_STRUCTURE)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
endif()

# Add module path
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/source/cmake)

# Prevent multiple inclusion of toxcore
if(NOT TARGET toxcore_static)
    add_subdirectory(third_party/c-toxcore)
endif()

# Add include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/c-toxcore
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/c-toxcore/toxcore
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    /opt/homebrew/include
)

# Add library directories
link_directories(
    /opt/homebrew/lib
)

# Find dependencies
find_package(Sodium REQUIRED)

# Add source directory
add_subdirectory(source)

# Add test directory if testing is enabled
option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
  add_subdirectory(test)
endif()
