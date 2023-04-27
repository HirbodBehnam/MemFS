cmake_minimum_required(VERSION 3.24)
project(MemFS C)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_C_STANDARD 11)

find_package(FUSE3 REQUIRED)

add_library(memfs_internal memfs.c)

add_executable(MemFS main.c)

target_include_directories(MemFS PRIVATE
        "${FUSE3_INCLUDE_DIR}"
        )
target_link_libraries(MemFS PRIVATE
        "${CMAKE_DL_LIBS}"
        "${FUSE3_LIBRARIES}"
        memfs_internal
        )

# Tests: https://coderefinery.github.io/cmake-workshop/testing/
add_executable(memfs_internal_tests memfs_test.c)
target_link_libraries(memfs_internal_tests PRIVATE memfs_internal)
enable_testing()
add_test(NAME memfs_internal_create_file COMMAND $<TARGET_FILE:memfs_internal_tests> 1)
add_test(NAME memfs_internal_create_folder COMMAND $<TARGET_FILE:memfs_internal_tests> 2)
add_test(NAME memfs_internal_get_entry COMMAND $<TARGET_FILE:memfs_internal_tests> 3)
add_test(NAME memfs_internal_read_write_file COMMAND $<TARGET_FILE:memfs_internal_tests> 4)
add_test(NAME memfs_internal_read_write_file_inflate COMMAND $<TARGET_FILE:memfs_internal_tests> 5)
add_test(NAME memfs_internal_resize_file COMMAND $<TARGET_FILE:memfs_internal_tests> 6)
add_test(NAME memfs_internal_delete_file COMMAND $<TARGET_FILE:memfs_internal_tests> 7)
add_test(NAME memfs_internal_delete_folder COMMAND $<TARGET_FILE:memfs_internal_tests> 8)