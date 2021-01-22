cmake_minimum_required(VERSION 3.17)
project(sourbar)

set(CMAKE_CXX_STANDARD 17)


add_executable(sourbar src/main.cc src/subprocess.cc src/modules.cc src/modules_lemonbar.cc)

target_include_directories(sourbar PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Threads
find_package(Threads REQUIRED)
target_link_libraries(sourbar Threads::Threads)

# i3ipc++
add_subdirectory(lib/i3ipc++)
target_include_directories(sourbar PRIVATE ${I3IPCpp_INCLUDE_DIRS})
target_link_directories(sourbar PRIVATE ${I3IPCpp_LIBRARY_DIRS})
target_link_libraries(sourbar ${I3IPCpp_LIBRARIES})


install(TARGETS sourbar DESTINATION ~/.local/bin)
