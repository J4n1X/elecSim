file(GLOB CXXFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_library(libElecSim STATIC ${CXXFILES})
target_include_directories(libElecSim PUBLIC 
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# Enable precompiled headers for faster compilation
target_precompile_headers(libElecSim PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../pch.h)

# Enable unity build for faster compilation
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.16")
    set_target_properties(libElecSim PROPERTIES UNITY_BUILD ON)
endif()

if(SIM_CACHING)
  target_compile_definitions(libElecSim PUBLIC SIM_CACHING)
endif()

set_property(TARGET libElecSim PROPERTY CXX_STANDARD 20)
set_property(TARGET libElecSim PROPERTY CXX_STANDARD_REQUIRED ON)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(libElecSim PRIVATE -Wall -Wextra -Wpedantic -Wno-unknown-pragmas)
  target_link_libraries(libElecSim PUBLIC olcPixelGameEngine nfd unordered_dense)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  add_compile_options(libElecSim PRIVATE /W3)
  target_link_libraries(libElecSim PUBLIC olcPixelGameEngine nfd unordered_dense)
endif()
