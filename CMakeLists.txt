cmake_minimum_required(VERSION 3.10)


# ==============================================================================
# Set the project name and version
project("Maintenance Planning Problem" VERSION 1.0.0 LANGUAGES CXX)


# ==============================================================================
# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)


# ==============================================================================
# Set the build type to Release by default if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()


# ==============================================================================
# External dependencies
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/modules/")

# Find Gurobi
find_package(GUROBI REQUIRED)

if(MSVC)
  if(MSVC_RUNTIME_LIBRARY STREQUAL "MultiThreaded" OR MSVC_RUNTIME_LIBRARY STREQUAL "MultiThreadedDebug")
    # Set the runtime library to Multi-Threaded (MT) for Release and Multi-Threaded Debug (MTd) for Debug.
    # This is necessary for MSVC to link against the correct Gurobi libraries and to avoid runtime errors 
    # related to the C++ standard library when using static linking.
    # The /MT flag is used for static linking, while /MD is used for dynamic linking.
    # The /MTd flag is used for debug builds with static linking, while /MDd is used for dynamic linking.
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")
  endif()
endif()


# ==============================================================================
# Path to header files
include_directories(
    ${GUROBI_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/lib
    ${PROJECT_SOURCE_DIR}/src
)


# ==============================================================================
# Source files
set(SOURCES
    src/main.cpp
    src/problem.cpp src/problem.hpp
    src/solver/relaxed_mip.cpp src/solver/relaxed_mip.hpp
    src/solver/de.cpp src/solver/de.hpp
)

# ==============================================================================
# Targets (executables)
add_executable(mpp ${SOURCES})
target_link_libraries(mpp optimized ${GUROBI_CXX_LIBRARY} ${GUROBI_LIBRARY}
                          debug ${GUROBI_CXX_DEBUG_LIBRARY} ${GUROBI_LIBRARY})
