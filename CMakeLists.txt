cmake_minimum_required(VERSION 3.17)
project(bar)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(lib/i3ipc++)

include_directories(${I3IPCpp_INCLUDE_DIRS})
link_directories(${I3IPCpp_LIBRARY_DIRS})

find_package(Threads REQUIRED)

add_executable(bar src/main.cpp src/subprocess.cc include/subprocess.h include/log.h src/modules.cc include/modules.h src/config.cc include/config.h src/modules_lemonbar.cc)
target_link_libraries(bar Threads::Threads)
target_link_libraries(bar ${I3IPCpp_LIBRARIES})
