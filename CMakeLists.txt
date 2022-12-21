cmake_minimum_required(VERSION 3.12.0)
project(AppleOrange)

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Freetype REQUIRED)

set(BIN_NAME apple_orange_problem)
set(SRC main.c data.c view.c proc.c sem.c list.c lib/glad.c)

add_executable(${BIN_NAME} ${SRC})

target_link_libraries(${BIN_NAME} ${OPENGL_LIBRARIES} glfw pthread)
