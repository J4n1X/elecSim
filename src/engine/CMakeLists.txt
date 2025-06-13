file(GLOB CXXFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_library(engine STATIC ${CXXFILES})
target_include_directories(engine PUBLIC 
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# Enable precompiled headers for faster compilation
target_precompile_headers(engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../pch.h)

# Enable unity build for faster compilation
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.16")
    set_target_properties(engine PROPERTIES UNITY_BUILD ON)
endif()

if(SIM_PREPROCESSING)
  target_compile_definitions(engine PUBLIC SIM_PREPROCESSING)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(engine PRIVATE -Wall -Wextra -Wpedantic -Wno-unknown-pragmas)
  target_link_libraries(engine PUBLIC olcPixelGameEngine nfd unordered_dense)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  add_compile_options(engine PRIVATE /W3)
  target_link_libraries(engine PUBLIC olcPixelGameEngine nfd unordered_dense)
endif()
