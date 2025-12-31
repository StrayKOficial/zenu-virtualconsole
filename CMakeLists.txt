cmake_minimum_required(VERSION 3.10)
project(zenu-emulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SDL2
find_package(SDL2 REQUIRED)

include_directories(emulator/include)

add_executable(zenu-emulator
    emulator/src/main.cpp
    emulator/src/bus.cpp
    emulator/src/cpu.cpp
    emulator/src/gpu.cpp
    emulator/src/loader.cpp
    emulator/src/apu.cpp
)

target_link_libraries(zenu-emulator SDL2::SDL2)
