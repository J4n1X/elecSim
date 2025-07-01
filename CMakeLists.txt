cmake_minimum_required(VERSION 3.20)
project("elecSim")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_UNITY_BUILD_BATCH_SIZE 8)
cmake_policy(SET CMP0140 NEW)
cmake_policy(SET CMP0135 NEW)

include(ProcessorCount)
ProcessorCount(NUM_PRCESSORS)

set(TESTS_DIR ${PROJECT_SOURCE_DIR}/examples/tests)
set(BUILD_DIR ${CMAKE_BINARY_DIR})
set(ASSETS_DIR ${PROJECT_SOURCE_DIR}/media)
set(TOOLS_DIR ${BUILD_DIR}/early_build/bin)

# Download cmrc (If we ever need it, I'll uncomment this)
#file(DOWNLOAD "https://raw.githubusercontent.com/vector-of-bool/cmrc/refs/heads/master/CMakeRC.cmake"
#"${CMAKE_BINARY_DIR}/CMakeRC.cmake")
#include("${CMAKE_BINARY_DIR}/CMakeRC.cmake")

# Debug and Release configuration options
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -UDEBUG")

# Enable comprehensive compiler warnings
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wcast-align -Wcast-qual -Wctor-dtor-privacy")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wdisabled-optimization -Wformat=2 -Winit-self")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wnoexcept -Wold-style-cast -Woverloaded-virtual")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wstrict-null-sentinel -Wstrict-overflow=5 -Wswitch-default")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wundef -Wunused -Wvariadic-macros -Wno-missing-field-initializers")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wwrite-strings -Wno-unknown-pragmas")
    
    # Additional GCC-specific warnings
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wduplicated-cond -Wduplicated-branches")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wnull-dereference -Wuseless-cast")
    endif()
    
    # Additional Clang-specific warnings
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-c++98-compat -Wno-c++98-compat-pedantic")
    endif()
    
    message(STATUS "Comprehensive compiler warnings enabled")
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

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()
# Ensure proper configuration types are available for multi-config generators (like Visual Studio)
if(CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Available build configurations" FORCE)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

if(EARLY_BUILD)
  message(STATUS "Building tools needed to configure the project")
  # Fetch Dependencies
  include(${CMAKE_SOURCE_DIR}/cmake/early_deps.cmake)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tools)
  return()
endif()

# Prepare needed tools for the project's build step
find_program(BLOBBER blobber PATHS ${TOOLS_DIR} PATH_SUFFIXES Debug Release)
if(WIN32 AND NOT BLOBBER)
  find_program(BLOBBER blobber.exe PATHS ${TOOLS_DIR} PATH_SUFFIXES Debug Release)
endif()
if(NOT BLOBBER_FOUND)
  message(STATUS "Preparing configuration step tools")
  set(workdir ${BUILD_DIR}/early_build)
  execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory "${workdir}")
  execute_process(COMMAND ${CMAKE_COMMAND} 
                  -G "${CMAKE_GENERATOR}"
                  -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
                  -DCMAKE_BUILD_TYPE=Release
                  -DEARLY_BUILD=ON
                  ${CMAKE_SOURCE_DIR}
                  WORKING_DIRECTORY "${workdir}")
  execute_process(COMMAND ${CMAKE_COMMAND} --build . -j ${NUM_PRCESSORS}
                  WORKING_DIRECTORY "${workdir}")
  find_program(BLOBBER blobber PATHS ${TOOLS_DIR})
endif()

# Add the seperate building of tools as a target
add_custom_target(tools ALL
  COMMAND ${CMAKE_COMMAND} --build ${BUILD_DIR}/early_build  -j ${NUM_PRCESSORS}
)

# Dependencies for the project
include(${CMAKE_SOURCE_DIR}/cmake/deps.cmake)

# Create the resource target (if we ever use more than just a single file. I'll keep it in here for now.)
# cmrc_add_resource_library(game-resources ALIAS gameResources::rc ${PROJECT_SOURCE_DIR}/media/BAHNSCHRIFT.TTF)

add_subdirectory(${PROJECT_SOURCE_DIR}/src)

enable_testing()
add_test(NAME component_test COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/prober -f ${TESTS_DIR}/componentTest.grid -t ${TESTS_DIR}/componentTest.probe -v)
add_test(NAME fulladder_test COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/prober -f ${TESTS_DIR}/fulladderTest.grid -t ${TESTS_DIR}/fulladderTest.probe -v)
# run all tests
