include(FetchContent)

# -----------------------------------------------------------------------------
# Early Build Dependencies
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