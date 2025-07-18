cmake_minimum_required(VERSION 3.4.1)

# Binary name
set(BINARY_NAME "tim2tox")

# Compiler warnings
add_definitions(-Wfatal-errors)

# Find dependencies
find_package(Sodium REQUIRED)

# Include paths
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/c-toxcore
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/c-toxcore/toxcore
    ${CMAKE_CURRENT_BINARY_DIR}/../third_party/c-toxcore
    ${CMAKE_CURRENT_BINARY_DIR}/../third_party/c-toxcore/toxcore
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/c-toxcore/build
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/c-toxcore/build/toxcore
    ${SODIUM_INCLUDE_DIR}
)

# Source files
file(GLOB TIM2TOX_SRCS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

# Handle ToxAV files based on BUILD_TOXAV option
if(NOT BUILD_TOXAV)
    list(FILTER TIM2TOX_SRCS EXCLUDE REGEX ".*ToxAVManager\\.(cpp|h)$")
endif()

# Create static library
add_library(${BINARY_NAME} STATIC ${TIM2TOX_SRCS})

# Add toxcore include directory
target_include_directories(${BINARY_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/third_party/c-toxcore/toxcore
    ${CMAKE_SOURCE_DIR}/third_party/c-toxcore
    ${CMAKE_BINARY_DIR}/third_party/c-toxcore
    ${CMAKE_BINARY_DIR}/third_party/c-toxcore/toxcore
)

# Link dependencies
target_link_libraries(${BINARY_NAME}
    PRIVATE
    toxcore_static
    ${SODIUM_LIBRARY}
)

if(BUILD_TOXAV)
    target_link_libraries(${BINARY_NAME}
        PRIVATE
        toxav_static
    )
endif()

# Compiler options
target_compile_options(${BINARY_NAME}
    PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wno-unused-parameter
    -fPIC
)

if(APPLE)
    target_compile_options(${BINARY_NAME}
        PRIVATE
        -Wno-deprecated-declarations
    )
endif()

# Version information
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/version.h"
)

# Install rules
install(TARGETS ${BINARY_NAME}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${BINARY_NAME}
    FILES_MATCHING PATTERN "*.h"
    PATTERN "*.h.in" EXCLUDE)