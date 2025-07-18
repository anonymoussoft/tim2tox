# Find toxav library
#
# This module defines the following variables:
# TOXAV_FOUND - True if toxav was found
# TOXAV_INCLUDE_DIRS - toxav include directories
# TOXAV_LIBRARIES - toxav libraries

find_path(TOXAV_INCLUDE_DIR
    NAMES tox/toxav.h
    PATHS
        /usr/include
        /usr/local/include
        /opt/local/include
        ${TOXAV_ROOT_DIR}/include
)

find_library(TOXAV_LIBRARY
    NAMES toxav libtoxav
    PATHS
        /usr/lib
        /usr/local/lib
        /opt/local/lib
        ${TOXAV_ROOT_DIR}/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Toxav
    REQUIRED_VARS
        TOXAV_LIBRARY
        TOXAV_INCLUDE_DIR
)

if(TOXAV_FOUND)
    set(TOXAV_LIBRARIES ${TOXAV_LIBRARY})
    set(TOXAV_INCLUDE_DIRS ${TOXAV_INCLUDE_DIR})
endif() 