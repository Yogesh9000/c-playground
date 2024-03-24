cmake_minimum_required(VERSION 3.10)

add_library(project_settings INTERFACE)
target_compile_features(project_settings INTERFACE cxx_std_23)

project(chip8)

find_package(fmt CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)

add_executable(chip8 main.cpp)

target_link_libraries(chip8 PRIVATE project_settings fmt::fmt)
target_link_libraries(chip8 PRIVATE
                           $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
                           $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>)
