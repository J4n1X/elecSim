# Function to blob something
function(_generate_blob target_name program path)
  set(BLOB_OUTPUT_DIR ${BUILD_DIR}/blobs/${target_name})
  file(MAKE_DIRECTORY ${BLOB_OUTPUT_DIR})

  if(NOT path)
    message(FATAL_ERROR "No path provided to generate_blob function")
  endif()
  if(NOT EXISTS ${path})
    message(FATAL_ERROR "Path does not exist: ${path}")
  endif()
  # Extract only the filename from the path (no extension)
  get_filename_component(FILENAME ${path} NAME)
  # Replace periods with underscores
  string(REPLACE "." "_" FILENAME "${FILENAME}_blob")
  
  set(BLOB_PATH "${BLOB_OUTPUT_DIR}/${FILENAME}.h")
  message(STATUS "Generating blob for ${path} at ${BLOB_PATH}")

  add_custom_command(
    OUTPUT ${BLOB_PATH}
    COMMAND ${program} -i ${path} -o ${BLOB_PATH}
    DEPENDS ${program} ${path}
    COMMENT "Generating blob for ${path} to ${BLOB_PATH}"
  )  
  execute_process(COMMAND ${program} -i ${path} -o ${BLOB_PATH})


  return(PROPAGATE BLOB_PATH BLOB_OUTPUT_DIR)
endfunction()

# Same function, but takes a list of paths
function(generate_blobs target_name program paths)
  if(NOT paths)
    message(FATAL_ERROR "No paths provided to generate_blobs function")
  endif()

  if(NOT EXISTS ${program})
    message(FATAL_ERROR "Program needed for blobbing does not exist: ${program}")
  endif()

  foreach(path IN LISTS paths)
    _generate_blob(${target_name} ${program} ${path})
    set(BLOB_FILES ${BLOB_FILES} ${BLOB_PATH})
  endforeach()

  set(CUSTOM_TARGET_NAME "${target_name}_BLOBS")
  add_custom_target(${CUSTOM_TARGET_NAME} ALL DEPENDS ${BLOB_FILES})
  target_include_directories(${target_name} PUBLIC ${BLOB_OUTPUT_DIR})
  add_dependencies(${target_name} ${CUSTOM_TARGET_NAME})
  return(PROPAGATE BLOB_FILES)
endfunction()