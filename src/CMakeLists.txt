file(GLOB CXXFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(elecSim ${CXXFILES})
set_property(TARGET elecSim PROPERTY CXX_STANDARD 20)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(elecSim PRIVATE -Wall -Wextra -Wpedantic -Wno-unknown-pragmas)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  add_compile_options(elecSim PRIVATE /W3)
endif()

target_include_directories(elecSim SYSTEM PUBLIC
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/include/olcPixelGameEngine
)

target_precompile_headers(elecSim PRIVATE
  ${PROJECT_SOURCE_DIR}/include/olcPixelGameEngine/olcPixelGameEngine.h
)

target_link_libraries(elecSim PRIVATE nfd)

