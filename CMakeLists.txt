cmake_minimum_required(VERSION 3.22)
project(learn_linux)

set(CMAKE_CXX_STANDARD 17)
add_subdirectory(learn_net)
add_executable(learn_linux main.cpp)
target_link_libraries(learn_linux fmt)