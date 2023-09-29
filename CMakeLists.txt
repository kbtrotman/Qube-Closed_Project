cmake_minimum_required(VERSION 3.0.0)
project(QubeFS VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_REQUIRED_LIBRARIES "pq" "crypto" "fuse3")

include(CTest)
enable_testing()

add_executable(${PROJECT_NAME}   qube.cpp
                                 qube.hpp
                                 q_psql.hpp
                                 q_fuse.hpp
                                 q_fuse.cpp
                                 q_fs_layer.hpp
                                 q_convert.hpp
                                 q_dedupe.hpp
                                 q_dedupe.cpp
                                 q_log.hpp
                                 q_log.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC 
   /home/trotmkeb/c/Qube/
   /usr/include 
   /usr/include/fuse3
   /usr/include/spdlog
   /usr/include/pq
)

target_link_libraries(${PROJECT_NAME} PUBLIC pq crypto fuse3)

target_compile_options(${PROJECT_NAME} PUBLIC -Wall -D_FILE_OFFSET_BITS=64 -pedantic-errors)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
