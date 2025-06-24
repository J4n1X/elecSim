# -----------------------------------------------------------------------------
# Subdirectories
# -----------------------------------------------------------------------------

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libElecSim)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/prober)

# -----------------------------------------------------------------------------
# Main target setup: elecSim
# -----------------------------------------------------------------------------

# Main project target
file(GLOB OUTER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB ENGINE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/engine/*.cpp)
add_executable(elecSim ${OUTER_FILES} ${ENGINE_FILES})
target_include_directories(elecSim PRIVATE 
  ${CMAKE_CURRENT_SOURCE_DIR}/engine
)
target_link_libraries(elecSim PRIVATE libElecSim nfd SFML::Graphics)

# Warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(elecSim PRIVATE -Wall -Wextra -Wpedantic)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(elecSim PRIVATE -O0 -g -fsanitize=address -fno-omit-frame-pointer -fno-inline -fno-optimize-sibling-calls)
    target_link_options(elecSim PRIVATE -fsanitize=address)
  endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  target_compile_options(elecSim PRIVATE /W3)
endif()


