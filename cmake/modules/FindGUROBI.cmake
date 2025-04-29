# This module finds the Gurobi library and includes directory.
# It sets the following variables:
# - GUROBI_INCLUDE_DIRS: The include directory for Gurobi.
# - GUROBI_LIBRARY: The Gurobi library.
# - GUROBI_CXX_LIBRARY: The Gurobi C++ library.
# - GUROBI_CXX_DEBUG_LIBRARY: The Gurobi C++ debug library.


# Find Gurobi include directory
find_path(GUROBI_INCLUDE_DIRS
    NAMES gurobi_c.h
    HINTS ${GUROBI_DIR} $ENV{GUROBI_HOME}
    PATH_SUFFIXES include)


# Find Gurobi library
find_library(GUROBI_LIBRARY
    NAMES gurobi gurobi100 gurobi110 gurobi120
    HINTS ${GUROBI_DIR} $ENV{GUROBI_HOME}
    PATH_SUFFIXES lib)


# Check if C++ is enabled to find Gurobi libraries for C++
if(CMAKE_CXX_COMPILER_LOADED)

    # Check if the compiler is Microsoft Visual Studio (MSVC)
    if(MSVC)

        # Check if the compiler is MSVC 2017 or later
        if(MSVC_VERSION VERSION_LESS 1910)
            message(FATAL_ERROR "Gurobi requires MSVC version 2017 or later.")
        endif()

        # Fix the MSVC version (year) to 2017, since this is the version used to name the 
        # Gurobi compiled libraries for Windows systems
        set(MSVC_YEAR "2017")

        # Check if the compiler is using Multi-Threaded Static (MT) or Multi-Threaded DLL (MD)
        if(MSVC_RUNTIME_LIBRARY STREQUAL "MultiThreaded" OR MSVC_RUNTIME_LIBRARY STREQUAL "MultiThreadedDebug")
            set(M_FLAG "mt")
        else()
            #if(MSVC_RUNTIME_LIBRARY STREQUAL "MultiThreadedDLL" OR MSVC_RUNTIME_LIBRARY STREQUAL "MultiThreadedDebugDLL")
            set(M_FLAG "md")
        endif()
        
        # Find the Gurobi libraries
        find_library(GUROBI_CXX_LIBRARY
            NAMES gurobi_c++${M_FLAG}${MSVC_YEAR}
            HINTS ${GUROBI_DIR} $ENV{GUROBI_HOME}
            PATH_SUFFIXES lib)

        # Find the Gurobi debug libraries
        # Note: The debug library name is different for MSVC
        find_library(GUROBI_CXX_DEBUG_LIBRARY
            NAMES gurobi_c++${M_FLAG}d${MSVC_YEAR}
            HINTS ${GUROBI_DIR} $ENV{GUROBI_HOME}
            PATH_SUFFIXES lib)

    # Check if the compiler is GCC
    else()
        find_library(GUROBI_CXX_LIBRARY
            NAMES gurobi_c++
            HINTS ${GUROBI_DIR} $ENV{GUROBI_HOME}
            PATH_SUFFIXES lib)
        set(GUROBI_CXX_DEBUG_LIBRARY ${GUROBI_CXX_LIBRARY})

    endif()
endif()


# Include the standard FindPackageHandleStandardArgs module
# This module provides a standard way to handle package finding
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GUROBI DEFAULT_MSG GUROBI_LIBRARY)
