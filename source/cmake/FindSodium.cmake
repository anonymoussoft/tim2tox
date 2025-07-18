# Find libsodium
# Find the native libsodium includes and library
#
# This module defines
#  SODIUM_INCLUDE_DIR, where to find sodium.h
#  SODIUM_LIBRARY, where to find the libsodium library
#  SODIUM_FOUND, If false, do not try to use libsodium

find_path(SODIUM_INCLUDE_DIR sodium.h
    PATHS
        /usr/local/include
        /opt/homebrew/include
        /usr/include
)

find_library(SODIUM_LIBRARY
    NAMES sodium libsodium
    PATHS
        /usr/local/lib
        /opt/homebrew/lib
        /usr/lib
        /usr/lib/x86_64-linux-gnu
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Sodium DEFAULT_MSG
    SODIUM_LIBRARY
    SODIUM_INCLUDE_DIR
)

mark_as_advanced(SODIUM_INCLUDE_DIR SODIUM_LIBRARY) 