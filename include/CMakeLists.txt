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

# Hope may be header-only, but it just sucks to link with C++, at least in this cmake project, don't ask me why
# This is my solution, and it works. Don't hate on me.
file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/hope/hope.c "#define HOPE_IMPLEMENTATION\n#include \"hope.h\"")
add_library(hope STATIC ${CMAKE_CURRENT_SOURCE_DIR}/hope/hope.c)
set_property(TARGET hope PROPERTY C_STANDARD 17)
set_property(TARGET hope PROPERTY C_STANDARD_REQUIRED ON)
target_include_directories(hope PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/hope
)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/nativefiledialog-extended)