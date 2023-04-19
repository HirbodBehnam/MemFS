cmake_minimum_required(VERSION 3.24)
project(CrowFS C)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_C_STANDARD 11)

find_package(FUSE3 REQUIRED)

add_executable(CrowFS main.c crowfs.c)

target_include_directories(CrowFS PRIVATE
        "${FUSE3_INCLUDE_DIR}"
        )
target_link_libraries(CrowFS PRIVATE
        "${CMAKE_DL_LIBS}"
        "${FUSE3_LIBRARIES}"
        )