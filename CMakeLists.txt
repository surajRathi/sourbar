cmake_minimum_required(VERSION 3.17)
project(bar)

set(CMAKE_CXX_STANDARD 17)

find_package(Threads REQUIRED)
add_executable(bar src/main.cpp src/subprocess.cc include/subprocess.h include/log.h src/modules.cc include/modules.h)
target_link_libraries(bar Threads::Threads)