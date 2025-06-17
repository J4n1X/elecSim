add_executable(prober ${CMAKE_CURRENT_SOURCE_DIR}/prober_main.cpp)
target_link_libraries(prober PRIVATE hope libElecSim )
target_include_directories(prober PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
)
# Warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(prober PRIVATE -Wall -Wextra -Wpedantic)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  target_compile_options(prober PRIVATE /W3)
endif()