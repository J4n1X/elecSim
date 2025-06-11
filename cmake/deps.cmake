include(FetchContent)

# -----------------------------------------------------------------------------
# Project setup: Grab dependencies.
# -----------------------------------------------------------------------------

# olcPixelGameEngine
FetchContent_Declare(
  olcPixelGameEngine
  GIT_REPOSITORY https://github.com/OneLoneCoder/olcPixelGameEngine
  GIT_TAG master
)
FetchContent_MakeAvailable(olcPixelGameEngine)

file(WRITE ${olcpixelgameengine_SOURCE_DIR}/olcPixelGameEngine.cpp 
"#define OLC_PGE_APPLICATION\n
#include \"olcPixelGameEngine.h\"
#define OLC_PGEX_QUICKGUI
#include \"extensions/olcPGEX_QuickGUI.h\""
)
add_library(olcPixelGameEngine STATIC ${olcpixelgameengine_SOURCE_DIR}/olcPixelGameEngine.cpp)
target_include_directories(olcPixelGameEngine SYSTEM PUBLIC
${olcpixelgameengine_SOURCE_DIR}
)

# Hope
FetchContent_Declare(
  hope
  GIT_REPOSITORY https://github.com/J4n1X/hope.git
  GIT_TAG main
)
FetchContent_MakeAvailable(hope)

file(WRITE ${hope_SOURCE_DIR}/hope.c 
"#define HOPE_IMPLEMENTATION\n
#include \"hope.h\""
)
add_library(hope STATIC ${hope_SOURCE_DIR}/hope.c)
set_property(TARGET hope PROPERTY C_STANDARD 17)
set_property(TARGET hope PROPERTY C_STANDARD_REQUIRED ON)
target_include_directories(hope SYSTEM PUBLIC
  ${hope_SOURCE_DIR}
)

# nativefiledialog-extended
FetchContent_Declare(
  nfd
  GIT_REPOSITORY https://github.com/btzy/nativefiledialog-extended
  GIT_TAG master
)
FetchContent_MakeAvailable(nfd)
# NFD already defines its own targets in its CMakeLists.txt, so we don't need to create one

FetchContent_Declare(unordered_dense
  GIT_REPOSITORY https://github.com/martinus/unordered_dense
  GIT_TAG main
)
FetchContent_MakeAvailable(unordered_dense)