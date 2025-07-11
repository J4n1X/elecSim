# -----------------------------------------------------------------------------
# Main target setup: elecSim
# -----------------------------------------------------------------------------

include(${CMAKE_SOURCE_DIR}/cmake/blobber.cmake)


# Main project target
file(GLOB OUTER_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB ENGINE_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/engine/*.cpp)

# Blob stuff
set(ASSETS 
  "${ASSETS_DIR}/BAHNSCHRIFT.TTF"
  "${ASSETS_DIR}/ArrowTile.mesh"
  "${ASSETS_DIR}/CrossingTile.mesh"
)

add_executable(elecSim ${OUTER_FILES} ${ENGINE_FILES})
target_include_directories(elecSim PRIVATE 
  ${CMAKE_CURRENT_SOURCE_DIR}/engine
)
#set_target_properties(elecSim PROPERTIES UNITY_BUILD ON)
target_link_libraries(elecSim PRIVATE libElecSim nfd SFML::Graphics ImGui-SFML::ImGui-SFML)
generate_blobs(elecSim ${BLOBBER} "${ASSETS}")
if(NOT BLOB_FILES)
  message(FATAL_ERROR "No blob files generated. Ensure the paths are correct and the blobber is working.")
endif()

# Warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(elecSim PRIVATE -Wall -Wextra -Wpedantic)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(ENABLE_MEMCHECK)
      target_compile_options(elecSim PRIVATE -O0 -g -fsanitize=address -fno-omit-frame-pointer -fno-inline -fno-optimize-sibling-calls)
      target_link_options(elecSim PRIVATE -fsanitize=address)
    endif()
  endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  target_compile_options(elecSim PRIVATE /W3)
endif()