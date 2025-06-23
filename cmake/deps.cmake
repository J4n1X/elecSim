include(FetchContent)

# -----------------------------------------------------------------------------
# Project setup: Grab dependencies.
# -----------------------------------------------------------------------------

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

# SFML 3.0.1
include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.1
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(SFML)