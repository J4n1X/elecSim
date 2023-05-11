if(${CMAKE_CXX_COMPILER} MATCHES ".*em[+][+]")
  set(PGE_USE_EMSCRIPTEN TRUE)
else()
  set(PGE_USE_EMSCRIPTEN FALSE)
endif()

if(PGE_USE_EMSCRIPTEN)
  # Compile to HTML5
  set(CMAKE_EXECUTABLE_SUFFIX .html)
endif()

# Asset folder
set(PGE_ASSET_FOLDER "${CMAKE_SOURCE_DIR}/assets")

# Copy the assets folder to the binary if it exists
if(EXISTS ${PGE_ASSET_FOLDER} AND IS_DIRECTORY ${PGE_ASSET_FOLDER})
  file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})
endif()

set(CMAKE_CXX_STANDARD 17)

# Sets the needed options for a specifig PGE target
#
# Usage: pge_set_emscripten_options(target)
function(pge_set_emscripten_options project_name)
  if(PGE_USE_EMSCRIPTEN)
    set(PGE_LINKER_OPTIONS -sALLOW_MEMORY_GROWTH=1 -sMAX_WEBGL_VERSION=2 -sMIN_WEBGL_VERSION=2 -sUSE_LIBPNG=1)

    if(PGE_INCLUDE_ASSETS)
      target_link_options(${project_name} PRIVATE ${PGE_LINKER_OPTIONS} --preload-file ${PGE_ASSET_FOLDER})
    else()
      target_link_options(${project_name} PRIVATE ${PGE_LINKER_OPTIONS})
    endif()
  else()
    if(UNIX)
      target_link_libraries(${project_name} PRIVATE -lX11 -lGL -lpthread -lpng -lstdc++fs)
    endif(UNIX)
  endif()
endfunction()