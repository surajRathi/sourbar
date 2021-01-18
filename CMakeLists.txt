cmake_minimum_required(VERSION 3.17)
project(sourbar)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(lib/i3ipc++)
include_directories(${I3IPCpp_INCLUDE_DIRS})
link_directories(${I3IPCpp_LIBRARY_DIRS})

find_package(Threads REQUIRED)

add_executable(sourbar src/main.cc src/subprocess.cc src/modules.cc src/modules_lemonbar.cc)

target_include_directories(sourbar PRIVATE ${include/})

target_link_libraries(sourbar Threads::Threads)
target_link_libraries(sourbar ${I3IPCpp_LIBRARIES})

install(TARGETS sourbar DESTINATION ~/.local/bin)
