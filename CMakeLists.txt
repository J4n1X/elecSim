cmake_minimum_required(VERSION 3.20)
project("elecSim")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(SIM_CACHING "Enable caching for simulation results" ON)

# Enable ccache if available for faster rebuilds
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set(CMAKE_CXX_COMPILER_LAUNCHER ccache)
    set(CMAKE_C_COMPILER_LAUNCHER ccache)
    message(STATUS "ccache found and enabled")
endif()

# Enable unity builds for faster compilation (when available)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.16")
    set(CMAKE_UNITY_BUILD_BATCH_SIZE 8)
    message(STATUS "Unity build support available")
endif()

# Check if we can do LTOs
include(CheckIPOSupported)
check_ipo_supported(RESULT lto_supported)
if(lto_supported)
  message(STATUS "Link Time Optimization (LTO) is supported")
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    message(STATUS "Link Time Optimization (LTO) enabled for Release build")
  else()
    message(STATUS "Link Time Optimization (LTO) not enabled for non-Release builds")
  endif()
endif()

include(${CMAKE_SOURCE_DIR}/cmake/deps.cmake)
#This has to be here for some reason, doesn't work in deps.cmake
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_link_libraries(olcPixelGameEngine PUBLIC X11 GL pthread png stdc++fs)
endif()


set(TESTS_DIR ${PROJECT_SOURCE_DIR}/examples/tests)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
message(STATUS "CMAKE_RUNTIME_OUTPUT_DIRECTORY: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()
# Ensure proper configuration types are available for multi-config generators (like Visual Studio)
if(CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Available build configurations" FORCE)
endif()

# Debug and Release configuration options
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -UDEBUG")

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

add_subdirectory(${PROJECT_SOURCE_DIR}/src)

enable_testing()
add_test(NAME component_test COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/prober -f ${TESTS_DIR}/componentTest.grid -t ${TESTS_DIR}/componentTest.probe -v)
add_test(NAME fulladder_test COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/prober -f ${TESTS_DIR}/fulladderTest.grid -t ${TESTS_DIR}/fulladderTest.probe -v)
# run all tests
