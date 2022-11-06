#include <glad/glad.h>
#include <GLFW/glfw3.h>

#ifndef font_struct
#define font_struct
struct font_texture {
  GLuint textureID;
  int width, height;
  int bearingX, bearingY;
  GLuint Advance;
}FTexture[128];
#endif