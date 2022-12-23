cmake_minimum_required(VERSION 3.12.0)
project(AppleOrange)

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Freetype REQUIRED)

include_directories(inc)

set(BIN_NAME apple_orange_problem)
aux_source_directory(lib SRC)

add_executable(${BIN_NAME} main.c ${SRC})

target_link_libraries(${BIN_NAME} ${OPENGL_LIBRARIES} glfw pthread)
