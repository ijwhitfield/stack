cmake_minimum_required(VERSION 3.25)
project(stack C)

set(CMAKE_C_STANDARD 99)

add_executable(stack main.c)

target_include_directories(stack PUBLIC /opt/homebrew/Cellar/raylib/4.5.0/include)
target_link_directories(stack PUBLIC /opt/homebrew/Cellar/raylib/4.5.0/lib)
target_link_libraries(stack raylib)

add_custom_target(copy_resources ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${PROJECT_SOURCE_DIR}/resources
        ${PROJECT_BINARY_DIR}/resources
        COMMENT "Copying resources into binary directory")

add_dependencies(stack copy_resources)

