file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/olcPixelGameEngine/olcPixelGameEngine.cpp 
"#define OLC_PGE_APPLICATION\n
#include \"olcPixelGameEngine.h\"
#define OLC_PGEX_QUICKGUI
#include \"extensions/olcPGEX_QuickGUI.h\""
)
add_library(olcPixelGameEngine STATIC ${CMAKE_CURRENT_SOURCE_DIR}/olcPixelGameEngine/olcPixelGameEngine.cpp)
target_include_directories(olcPixelGameEngine SYSTEM PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/olcPixelGameEngine
)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_link_libraries(olcPixelGameEngine PRIVATE X11 GL pthread png stdc++fs)
endif()

file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/hope/hope.c 
"#define HOPE_IMPLEMENTATION\n
#include \"hope.h\""
)
add_library(hope STATIC ${CMAKE_CURRENT_SOURCE_DIR}/hope/hope.c)
set_property(TARGET hope PROPERTY C_STANDARD 17)
set_property(TARGET hope PROPERTY C_STANDARD_REQUIRED ON)
target_include_directories(hope SYSTEM PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/hope
)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/nativefiledialog-extended)