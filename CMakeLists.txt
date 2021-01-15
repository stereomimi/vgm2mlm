cmake_minimum_required(VERSION 3.10)

project(vgm2mlm)

add_subdirectory(src/lib)

add_executable(vgm2mlm src/main.cpp)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)


target_link_libraries(vgm2mlm vgm2mlm_lib)
target_link_libraries(vgm2mlm stdc++fs)
