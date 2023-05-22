cmake_minimum_required(VERSION 3.8)

project("elecSim")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(PROJECT_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -Wno-unknown-pragmas)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  add_compile_options(/W4)
endif()

file(GLOB CXXFILES ${PROJECT_SOURCE_DIR}/*.cpp)
add_executable(elecSim ${CXXFILES})
set_property(TARGET elecSim PROPERTY CXX_STANDARD 20)


add_custom_target(
  run
  COMMAND elecSim
  DEPENDS elecSim
  WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

include(cmake/PGE.cmake)
pge_set_emscripten_options(elecSim)