cmake_minimum_required(VERSION 3.20)
project("elecSim")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(TESTS_DIR ${PROJECT_SOURCE_DIR}/examples/tests)
set(CMAKE_UNITY_BUILD_BATCH_SIZE 8)
# Debug and Release configuration options
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -UDEBUG")

if (WIN32)
    # /ENTRY:mainCRTStartup keeps the same "main" function instead of requiring "WinMain"
    set(SUBSYSTEM_LINKER_OPTIONS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
else()
    set(SUBSYSTEM_LINKER_OPTIONS "-mwindows")
endif()

# These are some options that the user can set
option(SIM_PREPROCESSING "Enable caching for simulation results" ON)
option(DISABLE_CCACHE "Disable ccache for builds" OFF)
option(DISABLE_LTO "Disable LTO for builds" OFF)

# Enable ccache if available for faster rebuilds
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND AND NOT DISABLE_CCACHE)
    set(CMAKE_CXX_COMPILER_LAUNCHER ccache)
    set(CMAKE_C_COMPILER_LAUNCHER ccache)
    message(STATUS "ccache found and enabled")
endif()

# Check if we can do LTOs
include(CheckIPOSupported)
check_ipo_supported(RESULT lto_supported)
if(NOT lto_supported)
  message(WARNING "LTO is not supported by this compiler")
else()
  message(STATUS "LTO is supported by this compiler")
endif()
if(lto_supported)
  if(NOT DISABLE_LTO)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    message(STATUS "LTO enabled for this project")
  else()
    message(STATUS "LTO disabled for this project")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION FALSE)
  endif()
endif()

include(${CMAKE_SOURCE_DIR}/cmake/deps.cmake)
#This has to be here for some reason, doesn't work in deps.cmake
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_link_libraries(olcPixelGameEngine PUBLIC X11 GL pthread png stdc++fs)
endif()


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


message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

add_subdirectory(${PROJECT_SOURCE_DIR}/src)

enable_testing()
add_test(NAME component_test COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/prober -f ${TESTS_DIR}/componentTest.grid -t ${TESTS_DIR}/componentTest.probe -v)
add_test(NAME fulladder_test COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/prober -f ${TESTS_DIR}/fulladderTest.grid -t ${TESTS_DIR}/fulladderTest.probe -v)
# run all tests
