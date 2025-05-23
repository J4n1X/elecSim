file(GLOB CXXFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(elecSim ${CXXFILES})
set_property(TARGET elecSim PROPERTY CXX_STANDARD 20)
set_property(TARGET elecSim PROPERTY CXX_STANDARD_REQUIRED ON)
target_link_libraries(elecSim PRIVATE libElecSim)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libElecSim)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/prober)

