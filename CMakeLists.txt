cmake_minimum_required(VERSION 3.0.0)
project(Qube VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_REQUIRED_LIBRARIES "pq" "crypto" "fuse3")

include_directories(.)

include(CTest)
enable_testing()

add_executable(qubeFS   qube.cpp
                        qube.hpp
                        q_psql.hpp
                        q_fuse.hpp
                        q_fs_layer.hpp
)

target_include_directories(qubeFS PUBLIC 
   /usr/include/fuse3
   /usr/include/spdlog
)

target_link_libraries(qubeFS PUBLIC pq crypto fuse3)

target_compile_options(qubeFS PUBLIC -Wall -D_FILE_OFFSET_BITS=64 -pedantic-errors)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
