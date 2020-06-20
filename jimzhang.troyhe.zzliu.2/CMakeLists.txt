cmake_minimum_required(VERSION 3.12)
project(p2)

include_directories(include)

set(CMAKE_CXX_STANDARD 17)
add_executable(p2 src/cpu.cpp src/main.cpp)
target_link_libraries(p2 ${CMAKE_CURRENT_SOURCE_DIR}/lib/libcpu.o)