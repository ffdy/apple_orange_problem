cmake_minimum_required(VERSION 3.12.0)
project(AppleOrange)

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Freetype REQUIRED)

set(BIN_NAME apple_orange1)
set(SRC main.c data.c view.c proc.c lib/glad.c lib/sem.c)

add_executable(${BIN_NAME} ${SRC})

target_link_libraries(${BIN_NAME} ${OPENGL_LIBRARIES} glfw pthread)
