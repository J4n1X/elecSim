file(GLOB CXXFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(elecSim ${CXXFILES})
set_property(TARGET elecSim PROPERTY CXX_STANDARD 20)
set_property(TARGET elecSim PROPERTY CXX_STANDARD_REQUIRED ON)
target_link_libraries(elecSim PRIVATE libElecSim)

# Warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(elecSim PRIVATE -Wall -Wextra -Wpedantic)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  target_compile_options(elecSim PRIVATE /W3)
endif()


add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libElecSim)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/prober)

