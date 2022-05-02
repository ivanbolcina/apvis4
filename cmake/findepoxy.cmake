find_package(PkgConfig QUIET)
pkg_check_modules(PC_LIBEPOXY QUIET epoxy)

if (PC_LIBEPOXY_FOUND)
    set(LIBEPOXY_DEFINITIONS ${PC_LIBEPOXY_CFLAGS_OTHER})
endif ()

find_path(LIBEPOXY_INCLUDE_DIRS
        NAMES epoxy/gl.h
        HINTS ${PC_LIBEPOXY_INCLUDEDIR} ${PC_LIBEPOXY_INCLUDE_DIRS}
        )

find_library(LIBEPOXY_LIBRARIES
        NAMES epoxy
        HINTS ${PC_LIBEPOXY_LIBDIR} ${PC_LIBEPOXY_LIBRARY_DIRS}
        )

mark_as_advanced(LIBEPOXY_INCLUDE_DIRS LIBEPOXY_LIBRARIES)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibEpoxy REQUIRED_VARS LIBEPOXY_INCLUDE_DIRS LIBEPOXY_LIBRARIES
        VERSION_VAR   PC_LIBEPOXY_VERSION)