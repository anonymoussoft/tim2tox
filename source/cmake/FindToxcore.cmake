# Find toxcore library
#
# This module defines the following variables:
# TOXCORE_FOUND - True if toxcore was found
# TOXCORE_INCLUDE_DIRS - toxcore include directories
# TOXCORE_LIBRARIES - toxcore libraries

find_path(TOXCORE_INCLUDE_DIR
    NAMES tox/tox.h
    PATHS
        /usr/include
        /usr/local/include
        /opt/local/include
        ${TOXCORE_ROOT_DIR}/include
)

find_library(TOXCORE_LIBRARY
    NAMES toxcore libtoxcore
    PATHS
        /usr/lib
        /usr/local/lib
        /opt/local/lib
        ${TOXCORE_ROOT_DIR}/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Toxcore
    REQUIRED_VARS
        TOXCORE_LIBRARY
        TOXCORE_INCLUDE_DIR
)

if(TOXCORE_FOUND)
    set(TOXCORE_LIBRARIES ${TOXCORE_LIBRARY})
    set(TOXCORE_INCLUDE_DIRS ${TOXCORE_INCLUDE_DIR})
endif() 