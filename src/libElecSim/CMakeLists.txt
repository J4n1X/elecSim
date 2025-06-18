file(GLOB CXXFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_library(libElecSim STATIC ${CXXFILES})
target_include_directories(libElecSim PUBLIC 
  ${CMAKE_CURRENT_SOURCE_DIR}
)

if(SIM_PREPROCESSING)
  target_compile_definitions(libElecSim PUBLIC SIM_PREPROCESSING)
  message(STATUS "Using simulation preprocessing")
else()
  message(STATUS "Using legacy, tile-by-tile simulation")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(libElecSim PRIVATE -Werror -Wall -Wextra -Wpedantic -Wno-unknown-pragmas)
  target_link_libraries(libElecSim PUBLIC unordered_dense)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  add_compile_options(libElecSim PRIVATE /W3)
  target_link_libraries(libElecSim PUBLIC unordered_dense)
endif()
