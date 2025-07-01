include(FetchContent)

# -----------------------------------------------------------------------------
# Project setup: Grab dependencies.
# -----------------------------------------------------------------------------

# Hope
FetchContent_Declare(
  hope
  GIT_REPOSITORY https://github.com/J4n1X/hope.git
  GIT_TAG main
  GIT_SHALLOW TRUE
)
# nativefiledialog-extended
FetchContent_Declare(
  nfd
  GIT_REPOSITORY https://github.com/btzy/nativefiledialog-extended
  GIT_TAG master
  GIT_SHALLOW TRUE
)
# NFD already defines its own targets in its CMakeLists.txt, so we don't need to create one

FetchContent_Declare(unordered_dense
  GIT_REPOSITORY https://github.com/martinus/unordered_dense
  GIT_TAG main
  GIT_SHALLOW TRUE
)

# SFML 3.0.1
FetchContent_Declare(
  SFML
  URL https://github.com/SFML/SFML/archive/3.0.1.zip
)

# ImGui
FetchContent_Declare(
  imgui
  URL https://github.com/ocornut/imgui/archive/v1.92.0.zip
)

# We don't need network support for SFML, and neither do we need autio
option(SFML_BUILD_NETWORK "Build the SFML network module" OFF)
option(SFML_BUILD_AUDIO "Build the SFML audio module" OFF)


# Could use Imgui-smfl later, using this template https://github.com/SFML/cmake-sfml-project/blob/imgui-sfml/CMakeLists.txt

FetchContent_MakeAvailable(nfd)
FetchContent_MakeAvailable(unordered_dense)
FetchContent_MakeAvailable(SFML)
FetchContent_MakeAvailable(imgui)

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