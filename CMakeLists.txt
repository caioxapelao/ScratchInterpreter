cmake_minimum_required(VERSION 3.16)

project(ScratchInterpreter)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(libzip REQUIRED)
add_executable(ScratchInterpreter src/main.cpp)
target_link_libraries(ScratchInterpreter PRIVATE libzip::zip)
target_include_directories(ScratchInterpreter PRIVATE include)
