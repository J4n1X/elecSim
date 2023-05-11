cmake_minimum_required(VERSION 3.8)

project("elecSim")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(PROJECT_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb -Werror -Wno-unknown-pragmas")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB CXXFILES ${PROJECT_SOURCE_DIR}/*.cpp)
add_executable(elecSim ${CXXFILES})
set_property(TARGET elecSim PROPERTY CXX_STANDARD 17)

add_custom_target(
  run
  COMMAND elecSim
  DEPENDS elecSim
  WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

include(cmake/PGE.cmake)
pge_set_emscripten_options(elecSim)