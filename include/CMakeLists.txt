add_library(olcPixelGameEngine INTERFACE)
target_include_directories(olcPixelGameEngine INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/olcPixelGameEngine
)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_link_libraries(olcPixelGameEngine INTERFACE X11 GL pthread png stdc++fs)
endif()
target_precompile_headers(olcPixelGameEngine INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/olcPixelGameEngine/olcPixelGameEngine.h
)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/hope)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/nativefiledialog-extended)