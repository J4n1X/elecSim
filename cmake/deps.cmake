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
  URL https://github.com/ocornut/imgui/archive/v1.91.5.zip
)

# ImGui-SFML
FetchContent_Declare(
  imgui-sfml
  GIT_REPOSITORY https://github.com/SFML/imgui-sfml
  GIT_COMMIT f768c96c3a8ff8ca9817ea1d1ce597162e7e714c
  GIT_SHALLOW TRUE
)

# We don't need network support for SFML, and neither do we need autio
option(SFML_BUILD_NETWORK "Build the SFML network module" OFF)
option(SFML_BUILD_AUDIO "Build the SFML audio module" OFF)


# Could use Imgui-smfl later, using this template https://github.com/SFML/cmake-sfml-project/blob/imgui-sfml/CMakeLists.txt

FetchContent_MakeAvailable(nfd)
FetchContent_MakeAvailable(unordered_dense)
FetchContent_MakeAvailable(SFML)
FetchContent_MakeAvailable(imgui)

# ImGui-SFML
set(IMGUI_DIR ${imgui_SOURCE_DIR})
option(IMGUI_SFML_FIND_SFML "Use find_package to find SFML" OFF)
option(IMGUI_SFML_IMGUI_DEMO "Build imgui_demo.cpp" OFF)
FetchContent_MakeAvailable(imgui-sfml)

# Suppress warnings for external dependencies
if(TARGET ImGui-SFML)
    target_compile_options(ImGui-SFML PRIVATE -w)
endif()
if(TARGET nfd)
    target_compile_options(nfd PRIVATE -w)
endif()
if(TARGET sfml-system)
    target_compile_options(sfml-system PRIVATE -w)
endif()
if(TARGET sfml-window)
    target_compile_options(sfml-window PRIVATE -w)
endif()
if(TARGET sfml-graphics)
    target_compile_options(sfml-graphics PRIVATE -w)
endif()


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