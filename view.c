#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "data.h"
#include "view.h"

// 图形变化
#define MATH_3D_IMPLEMENTATION
#include "lib/math_3d.h"
// 纹理导入
#define STB_IMAGE_IMPLEMENTATION
#include "lib/stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "lib/stb_truetype.h"

pthread_t view_thread;

float pcVertices[] = {
  //-坐标----  纹理坐标----
  0.0f, 0.3f, 0.0f, 1.0f,
  0.0f, 0.0f, 0.0f, 0.0f,
  0.3f, 0.0f, 1.0f, 0.0f,
  0.3f, 0.3f, 1.0f, 1.0f 
};

float memVertices[] = {
  0.0f, 0.4f, 0.0f, 1.0f,
  0.0f, 0.0f, 0.0f, 0.0f,
  0.4f, 0.0f, 1.0f, 0.0f,
  0.4f, 0.4f, 1.0f, 1.0f
};

unsigned int indices[] = {
  0, 1, 3,
  1, 2, 3
};

float pcOffset[N][4][2] = {
  -0.7f,  0.4f,
  -0.7f, -0.7f,
   0.4f, -0.7f,
   0.4f,  0.4f
};

float memOffset[N][2] = {
  -0.2f, -0.2f
};

GLuint VAO[2], VBO[2], EBO[2];
GLuint shaderProgram[3];
GLuint fontShaderProgram;

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

void genTextureFromFile(GLuint textureIndex, GLuint textureID, const char *imgPath) {
  glActiveTexture(textureIndex);
  glBindTexture(GL_TEXTURE_2D, textureID);
   // 设定对象环绕、过滤方式
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  // 加载图片
  int width, height, nrChannels;
  unsigned char *data =
      stbi_load(imgPath, &width, &height, &nrChannels, 0);
  if (data) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
               GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D); // 自动生成多级渐远纹理
  } else {
    printf("image load fail;\n");
  }
  stbi_image_free(data);
}

void genTextureFromColor(GLuint textureIndex, GLuint textureID,
                         vec3_t color) {
  glActiveTexture(textureIndex);
  glBindTexture(GL_TEXTURE_2D, textureID);
   // 设定对象环绕、过滤方式
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  // 加载图片
  unsigned char data[3 * 250 * 250 * sizeof(unsigned char)];
  for (int i = 0; i < 250 * 250; i++) {
    data[i * 3] = (unsigned char)(color.x * 255.0f);
    data[i * 3 + 1] = (unsigned char)(color.y * 255.0f);
    data[i * 3 + 2] = (unsigned char)(color.z * 255.0f);
  }
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 250, 250, 0, GL_RGB,
             GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D); // 自动生成多级渐远纹理
}

GLubyte fontBuffer[1 << 20];
GLubyte temp_bitmap[1 << 20];
stbtt_bakedchar cdata[96];
void genTextureFromTTF(GLuint textureIndex, GLuint textureID,
                       const char *ttf_path, GLfloat font_height) {
  stbtt_BakeFontBitmap(fontBuffer, 0, font_height, temp_bitmap, 1024, 1024, 32,
                       96, cdata);
  glActiveTexture(textureIndex);
  glBindTexture(GL_TEXTURE_2D, textureID);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 1024, 1024, 0, GL_ALPHA,
               GL_UNSIGNED_BYTE, temp_bitmap);
  glGenerateMipmap(GL_TEXTURE_2D);
}
// todo
void drawFont() {
  glUseProgram(fontShaderProgram);
  GLuint FVAO, FVBO;
  

  // glad_glUniform2f(glad_glGetUniformLocation(fontShaderProgram, "vertex"), )
}

void drawOnce(GLFWwindow *window) {
  glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  glUseProgram(shaderProgram[0]);
  glBindVertexArray(VAO[1]);

  for (int i = 0; i < N; i++) {
    for (int j = 0; j < 4; j++) {
      glUniform2f(glGetUniformLocation(shaderProgram[0], "offset"), pcOffset[i][j][0],
                  pcOffset[i][j][1]);
      // glUniform1i(glGetUniformLocation(shaderProgram[0], "textureImg"), 0);
      // glUniform1i(glGetUniformLocation(shaderProgram[2], "textureImg"), 1);
      glUniform3f(glGetUniformLocation(shaderProgram[0], "textureColor"), workColor[pcState[i][j]][0], workColor[pcState[i][j]][1], workColor[pcState[i][j]][2]);
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);  
    }
  }

  for (int i = 0; i < N; i++) {
    if (memState[i] == 0) {
      glUseProgram(shaderProgram[0]);
      glUniform3f(glGetUniformLocation(shaderProgram[0], "textureColor"), workColor[0][0], workColor[0][1], workColor[0][2]);
    } else if (memState[i] == 1) {
      glUseProgram(shaderProgram[1]);
      glUniform1i(glGetUniformLocation(shaderProgram[1], "textureImg"), 0);
    } else if (memState[i] == 2) {
      glUseProgram(shaderProgram[2]);
      glUniform1i(glGetUniformLocation(shaderProgram[2], "textureImg"), 0);
    } else if (memState[i] == 3) {
      glUseProgram(shaderProgram[1]);
      glUniform1i(glGetUniformLocation(shaderProgram[1], "textureImg"), 1);
    } else if (memState[i] == 4) {
      glUseProgram(shaderProgram[2]);
      glUniform1i(glGetUniformLocation(shaderProgram[2], "textureImg"), 1);
    }
    glBindVertexArray(VAO[0]);
    glUniform2f(glGetUniformLocation(shaderProgram[0], "offset"),
                memOffset[i][0], memOffset[i][1]);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glUseProgram(0);
  }

  glfwSwapBuffers(window);
  glfwPollEvents();
}

//键盘按键回调函数  
void processInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, 1);
}

void *view(void *arg) {
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  stbi_set_flip_vertically_on_load(1);

  GLFWwindow *window = glfwCreateWindow(800, 800, "Apple-Orange Problem 1.0", NULL, NULL);
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

  // 顶点着色器
  const char *vertexShaderSource =
      getShaderSource("../shader/vertexShader.glsl");
  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
  glCompileShader(vertexShader);
  int success;
  char infoLog[512];
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
    printf("vertex shader compile fail\n");
  }

  // GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  // compileShader("shader/vertexShader.glsl", vertexShader);
  const char *fontVertexShaderSource =
      getShaderSource("../shader/fontVertexShader.glsl");
  GLuint fontVertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(fontVertexShader, 1, &fontVertexShaderSource, NULL);
  glCompileShader(fontVertexShader);

  // 片段着色器1
  const char *fragmentShaderSource[3];
  fragmentShaderSource[0] = getShaderSource("../shader/fragmentShaderColor.glsl");
  fragmentShaderSource[1] = getShaderSource("../shader/fragmentShaderGray.glsl");
  fragmentShaderSource[2] = getShaderSource("../shader/fragmentShaderTexture.glsl");
  GLuint fragmentShader[3];
  for (int i = 0; i < 3; i++) {
    fragmentShader[i] = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader[i], 1, &fragmentShaderSource[i], NULL);
    glCompileShader(fragmentShader[i]);
    glGetShaderiv(fragmentShader[i], GL_COMPILE_STATUS, &success);
    if (!success) {
      glGetShaderInfoLog(fragmentShader[i], 512, NULL, infoLog);
      printf("fragment shader compile fail\n");
    }
    // GLuint fragmentShader[i] = glCreateShader(GL_FRAGMENT_SHADER);
    // compileShader("shader/fragmentShader[i].glsl", fragmentShader[i]);
  }

  const char *fontFragmentShaderSource =
      getShaderSource("../shader/fontFragmentShader.glsl");
  GLuint fontFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fontFragmentShader, 1, &fontFragmentShaderSource, NULL);
  glCompileShader(fontFragmentShader);

  // GLuint shaderProgram[3];
  for (int i = 0; i < 3; i++) {
    // 着色器程序
    shaderProgram[i] = glCreateProgram();
    glAttachShader(shaderProgram[i], vertexShader);
    glAttachShader(shaderProgram[i], fragmentShader[i]);
    glLinkProgram(shaderProgram[i]);
    glGetProgramiv(shaderProgram[i], GL_COMPILE_STATUS, &success);
    if (!success) {
      glGetProgramInfoLog(shaderProgram[i], 512, NULL, infoLog);
      printf("shader program link fail\n");
    }

    // glUseProgram(shaderProgram); //使用的时候调用
  }

  fontShaderProgram = glCreateProgram();
  glAttachShader(fontShaderProgram, fontVertexShader);
  glAttachShader(fontShaderProgram, fontFragmentShader);
  glLinkProgram(fontShaderProgram);
  // glGetProgramiv(fontShaderProgram, GL_COMPILE_STATUS)

  glDeleteShader(vertexShader);
  for (int i = 0; i < 3; i++) {
    glDeleteShader(fragmentShader[i]);
  }

  glDeleteShader(fontVertexShader);
  glDeleteShader(fontFragmentShader);

  glGenVertexArrays(2, VAO);
  glGenBuffers(2, VBO);
  glGenBuffers(2, EBO);

  glBindVertexArray(VAO[0]);
  glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(pcVertices), pcVertices, GL_DYNAMIC_DRAW);

  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                        (void *)(2 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO[0]);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
               GL_DYNAMIC_DRAW);

  glBindVertexArray(VAO[1]);
  glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(memVertices), memVertices, GL_DYNAMIC_DRAW);

  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                        (void *)(2 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO[1]);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
              GL_DYNAMIC_DRAW);

  // 纹理
  GLuint texture[3];
  glGenTextures(3, texture);
  genTextureFromFile(GL_TEXTURE0, texture[0], "../img/apple.jpg");
  genTextureFromFile(GL_TEXTURE1, texture[1], "../img/orange.jpg");
  genTextureFromTTF(GL_TEXTURE2, texture[2], "../ttf/CascadiaCode.ttf", 100);

  while (!glfwWindowShouldClose(window)) {
    processInput(window);
    drawOnce(window);
  }

  glDeleteBuffers(2, VAO);
  glDeleteBuffers(2, VBO);
  glDeleteBuffers(2, EBO);
  for (int i = 0; i < 3; i++) {
    glDeleteProgram(shaderProgram[i]);
  }
  glDeleteProgram(fontShaderProgram);

  glfwTerminate();

  return NULL;
}

void view_start() { pthread_create(&view_thread, NULL, view, NULL); }

void view_done() { pthread_join(view_thread, NULL); }
