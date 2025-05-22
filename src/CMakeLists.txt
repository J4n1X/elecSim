file(GLOB CXXFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(elecSim ${CXXFILES})
set_property(TARGET elecSim PROPERTY CXX_STANDARD 20)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(elecSim PRIVATE -Wall -Wextra -Wpedantic -Wno-unknown-pragmas)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  add_compile_options(elecSim PRIVATE /W3)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  target_compile_options(elecSim PRIVATE /MT)
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(elecSim PRIVATE /LTCG)
  endif()
endif()


target_include_directories(elecSim SYSTEM PUBLIC
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/include/PGE
)

target_link_libraries(elecSim PRIVATE PixelGameEngine nfd)

