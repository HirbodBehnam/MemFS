cmake_minimum_required(VERSION 3.24)
project(CrowFS C)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_C_STANDARD 11)

find_package(FUSE3 REQUIRED)

add_library(crowfs_internal crowfs.c)

add_executable(CrowFS main.c)

target_include_directories(CrowFS PRIVATE
        "${FUSE3_INCLUDE_DIR}"
        )
target_link_libraries(CrowFS PRIVATE
        "${CMAKE_DL_LIBS}"
        "${FUSE3_LIBRARIES}"
        crowfs_internal
        )

# Tests: https://coderefinery.github.io/cmake-workshop/testing/
add_executable(crowfs_internal_tests crowfs_test.c)
target_link_libraries(crowfs_internal_tests PRIVATE crowfs_internal)
enable_testing()
add_test(NAME crowfs_internal_create_file COMMAND $<TARGET_FILE:crowfs_internal_tests> 1)
add_test(NAME crowfs_internal_create_folder COMMAND $<TARGET_FILE:crowfs_internal_tests> 2)
add_test(NAME crowfs_internal_get_entry COMMAND $<TARGET_FILE:crowfs_internal_tests> 3)
add_test(NAME crowfs_internal_read_write_file COMMAND $<TARGET_FILE:crowfs_internal_tests> 4)
add_test(NAME crowfs_internal_read_write_file_inflate COMMAND $<TARGET_FILE:crowfs_internal_tests> 5)
add_test(NAME crowfs_internal_resize_file COMMAND $<TARGET_FILE:crowfs_internal_tests> 6)