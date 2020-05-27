cmake_minimum_required(VERSION 3.17)
project(bar)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(lib/i3ipc++)

include_directories(${I3IPCpp_INCLUDE_DIRS})
link_directories(${I3IPCpp_LIBRARY_DIRS})

find_package(Threads REQUIRED)

add_executable(bar src/main.cc src/subprocess.cc src/modules.cc src/modules_lemonbar.cc)

# add_executable(bar)
# target_include_directories(bar PRIVATE src/)

target_include_directories(bar PRIVATE ${include/})

target_link_libraries(bar Threads::Threads)
target_link_libraries(bar ${I3IPCpp_LIBRARIES})


install(TARGETS bar DESTINATION ~/.local/bin)
