cmake_minimum_required(VERSION 3.25)

add_executable(blobber blobber.c)
target_link_libraries(blobber PRIVATE hope)