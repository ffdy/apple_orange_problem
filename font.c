#include "font.h"
#include "view.h"

#include <ft2build.h>
#include <freetype/freetype.h>

#include <stdio.h>
#include <stdlib.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

FT_Library ft;
FT_Face face;

void font_init() {
  if (FT_Init_FreeType(&ft)) {
    printf("Could not init freetype library\n");
  }

  if (FT_New_Face(ft, "../ttf/CascadiaCode.ttf", 0, &face)) {
    printf("Failed to load font\n");
  }

  FT_Set_Pixel_Sizes(face, 0, 48);
  if (FT_Load_Char(face, 'X', FT_LOAD_RENDER)) {
    printf("Failed to load Glyph\n");
  }

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // 禁用字节对齐
}

void font_load() {
  for (GLubyte c = 0; c < 128; c++) {
    if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
      printf("Failed to load Glyph\n");
      continue;
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, face->glyph->bitmap.width,
                 face->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE,
                 face->glyph->bitmap.buffer);
    // 设置纹理选项
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    FTexture[c].textureID = texture;
    FTexture[c].width = face->glyph->bitmap.width;
    FTexture[c].height = face->glyph->bitmap.rows;
    FTexture[c].bearingX = face->glyph->bitmap_left;
    FTexture[c].bearingY = face->glyph->bitmap_top;
    FTexture[c].Advance = face->glyph->advance.x;
  }
}

void font_done() {
  FT_Done_Face(face);
  FT_Done_FreeType(ft);
}

GLFWwindow *window;
GLuint VAO, VBO;
GLuint shaderProgram;
GLuint vertexShader;
GLuint fragmentShader;

char *getShaderSource(const char *path) {
  FILE *file = fopen(path, "r");
  fseek(file, 0, SEEK_END);
  int file_size = ftell(file) + 1;
  rewind(file);

  char *shaderSource = malloc(file_size);
  file_size = fread(shaderSource, 1, file_size, file);
  fclose(file);
  shaderSource[file_size] = '\0';
  return shaderSource;
}

void compileShader(const char *glslPath, GLuint shaderID) {
  int success;
  char infoLog[512];
  const char *shaderSource = getShaderSource(glslPath);
  glShaderSource(shaderID, 1, &shaderSource, NULL);
  glCompileShader(shaderID);
  glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(shaderID, 512, NULL, infoLog);
    printf("shader compile fail\n");
  }
}

void RenderText(char *text, GLfloat x, GLfloat y, GLfloat scale) {
  glUseProgram(shaderProgram);
  glActiveTexture(GL_TEXTURE0);
  glBindVertexArray(VAO);

  for (char *c = text; *c != '\0'; c++) {
    
  }
}

void test_font_view() {
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

  window = glfwCreateWindow(800, 600, "Char", NULL, NULL);
  if (window == NULL) {
    printf("window create fail\n");
    glfwTerminate();
    exit(-1);
  }
  glfwMakeContextCurrent(window);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    printf("glad load fail\n");
    exit(-1);
  }

  glEnable(GL_CULL_FACE);
  // 启用混合（教程高级）
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  const char *vertexShaderSource =
      getShaderSource("../shader/fontVertexShader.glsl");
  vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
  glCompileShader(vertexShader);
  // 获取错误信息
  int success;
  char infoLog[512];
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
    printf("vertex shader compile fail\n");
  }

  const char *fragmentShaderSource =
      getShaderSource("../shader/fontFragmentShader.glsl");

  fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
  glCompileShader(fragmentShader);
  glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
    printf("fragment shader compile fail\n");
  }


  shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);
  glGetProgramiv(shaderProgram, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
    printf("shader program link fail\n");
  }

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  font_init();
  font_load();
  font_done();

  // vao vbo

  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
  // 取消绑定
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // RenderText()

    glfwSwapBuffers(window);
  }
}