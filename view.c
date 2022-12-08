#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "data.h"
#include "view.h"

// 图形变化
#define MATH_3D_IMPLEMENTATION
#include "lib/math_3d.h"
// 纹理导入
#define STB_IMAGE_IMPLEMENTATION
#include "lib/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "lib/stb_image_write.h"
// 文本渲染
#define STB_TRUETYPE_IMPLEMENTATION
#include "lib/stb_truetype.h"

float WW = 1600, WH = 900;

float bdx = 0, bdy = 0;

pthread_t view_thread;

float pcVertices[] = {
    //-坐标----  纹理坐标----
    // 0.0f, 0.3f, 0.0f, 1.0f,
    // 0.0f, 0.0f, 0.0f, 0.0f,
    // 0.3f, 0.0f, 1.0f, 0.0f,
    // 0.3f, 0.3f, 1.0f, 1.0f
    -0.741f, 0.926, 0.0f, 1.0f,
    -0.741f, 0.838, 0.0f, 0.0f, 
    -0.634f, 0.838, 1.0f, 0.0f,
    -0.634f, 0.926f, 1.0f, 1.0f
};

float memVertices[] = {
    // -0.732f, -0.059f, 0.0f, 1.0f,
    // -0.732f, -0.25f, 0.0f, 0.0f,
    // -0.518f, -0.25f, 1.0f, 0.0f,
    // -0.518f, -0.059f, 1.0f, 1.0f
    -0.727f, -0.147f, 0.0f, 1.0f,
    -0.727f, -0.368f, 0.0f, 0.0f,
    -0.591f, -0.368f, 1.0f, 0.0f,
    -0.591f, -0.147f, 1.0f, 1.0f
};

float buttonVertices[] = {
    0.636f, 0.353f, 0.0f, 0.0f,
    0.709f, 0.412f, 0.0f, 0.0f,
    0.709f, 0.294f, 0.0f, 0.0f,
    
    0.927f, 0.353f, 0.0f, 0.0f,
    0.854f, 0.412f, 0.0f, 0.0f,
    0.854f, 0.294f, 0.0f, 0.0f,
    
    0.636f, 0.118f, 0.0f, 0.0f,
    0.709f, 0.176f, 0.0f, 0.0f,
    0.709f, 0.059f, 0.0f, 0.0f,
    
    0.927f, 0.118f, 0.0f, 0.0f,
    0.854f, 0.176f, 0.0f, 0.0f,
    0.854f, 0.059f, 0.0f, 0.0f
};

unsigned int indices[] = {
  0, 1, 3,
  1, 2, 3
};

float pcOffset[N][4][2];
float memOffset[N][2];

float pcDw = 0.125f, pcDh = 0.118f;
// float memDw = 0.25f, memDh = 0.221f;
float memDw = 0.173f, memDh = 0.397f;

struct selected {
  int id, type;
  int *work_time, *free_time;
} Selected;

void view_data() {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 20; j += 2) {
      pcOffset[j][i][0] = (int)(j / 2) * pcDw;
      pcOffset[j + 1][i][0] = (int)(j / 2) * pcDw;
      pcOffset[j][i][1] = 0 - 2 * i * pcDh;
      pcOffset[j + 1][i][1] = 0 - (2 * i + 1) * pcDh;
    }

  }
  for (int i = 0; i < 20; i += 2) {
    // for (int k = 0; k < 4; k++) {
    //   memOffset[j + k][0] = (int)(j / 4) * memDw;
    //   memOffset[j + k][1] = 0 - k * memDh;
    // }
    memOffset[i][0] = (int)(i / 2) * memDw;
    memOffset[i][1] = 0;
    memOffset[i + 1][0] = (int)(i / 2) * memDw;
    memOffset[i + 1][1] = -memDh;
  }
}

GLuint VAO[2], VBO[2], EBO[2];
GLuint bVAO, bVBO;
GLuint shaderProgram[3];
GLuint fontShaderProgram;

int dotInTriangle(float x, float y, float x1, float y1,
                 float x2, float y2, float x3, float y3) {
  float s = fabsf((x2 - x1) * (y3 - y2) - (x3 - x1) * (y2 - y1)) / 2;
  float s1 = fabsf((x1 - x) * (y2 - y) - (x2 - x) * (y1 - y)) / 2;
  float s2 = fabsf((x1 - x) * (y3 - y) - (x3 - x) * (y1 - y)) / 2;
  float s3 = fabsf((x3 - x) * (y2 - y) - (x2 - x) * (y3 - y)) / 2;

  // printf("(%f, %f), (%f, %f), (%f, %f), (%f, %f) ", x, y, x1, y2, x2, y2, x3, y3);
  // printf("s: %f, s1: %f, s2: %f, s3: %f\n", s, s1, s2, s3);

  if (fabsf(s1 + s2 + s3 - s) < 0.01) {
    return 1;
  }
  return 0;
}

int dotInBox(float x, float y, float x1, float y1, float x2, float y2) {
  printf("(%f, %f), (%f, %f), (%f, %f)\n", x, y, x1, y1, x2, y2);
  if (x <= x2 && x >= x1 && y >= y2 && y <= y1)
    return 1;
  return 0;
}

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
GLubyte rgba[1 << 22];
stbtt_bakedchar cdata[96];
void genTextureFromTTF(GLuint textureIndex, GLuint textureID,
                       const char *ttf_path, GLfloat font_height) {
  // fread(fontBuffer, 1, 1 << 20, fopen(ttf_path, "rb"));
  stbtt_BakeFontBitmap(fontBuffer, 0, font_height, temp_bitmap, 1024, 1024, 32,
                       96, cdata);
  memset(rgba, 0, sizeof rgba);

  for (int i = 0; i < 1024 * 1024; i++) {
    rgba[4 * i + 3] = temp_bitmap[i];
  }
  // stbi_write_png("../img/STB.png", 1024, 1024, 1, temp_bitmap, 1024);
  glActiveTexture(textureIndex);
  glBindTexture(GL_TEXTURE_2D, textureID);
  // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1024, 1024, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, rgba);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  // glGenerateMipmap(GL_TEXTURE_2D);
  // printf("load over\n");
}
// todo
// +---->x
// |
// |
// |y
void drawFont(GLFWwindow *window, float x, float y, char *msg, float font_height) {
  GLuint tex1, tex2;
  glad_glGenTextures(1, &tex1);
  glad_glGenTextures(1, &tex2);

  x *= WW;
  y *= WH;

  genTextureFromTTF(GL_TEXTURE2, tex1, "../ttf/CascadiaCode.ttf",
                    font_height);
  // genTextureFromFile(GL_TEXTURE3, tex2, "../img/apple.jpg");

  glUseProgram(fontShaderProgram);
  GLuint FVAO, FVBO, FEBO;
  stbtt_aligned_quad q;
  // float fontVertices[16];
  glGenVertexArrays(1, &FVAO);
  glGenBuffers(1, &FVBO);
  glGenBuffers(1, &FEBO);

  glUniform1i(glGetUniformLocation(fontShaderProgram, "texImg"), 2);
  // glUniform1i(glGetUniformLocation(fontShaderProgram, "texImg1"), 0);

  for (char *c = msg; *c != '\0'; c++) {
    stbtt_GetBakedQuad(cdata, 1024, 1024, *c - 32, &x, &y, &q, 1);
    // font_width = (q.x1 - q.x0) * font_height / (q.y0 - q.y1);
    // printf("%f\t%f\t%f\t%f\n", q.s0, q.t0, q.s1, q.t1);
    // printf("%f\t%f\t%f\t%f\n", q.x0, q.y0, q.x1, q.y1);
    float fontVertices[] = {
      (q.x0 - WW) / WW, (-q.y0 - font_height + WH) / WH, q.s0, q.t0,
      (q.x0 - WW) / WW, (-q.y1 - font_height + WH) / WH, q.s0, q.t1,
      (q.x1 - WW) / WW, (-q.y1 - font_height + WH) / WH, q.s1, q.t1,
      (q.x1 - WW) / WW, (-q.y0 - font_height + WH) / WH, q.s1, q.t0
      // -1.0f, 1.0f, 0.0f, 1.0f,
      // -1.0f, -1.0f, 0.0f, 0.0f, 
      // 1.0f, -1.0f, 1.0f, 0.0f,
      // 1.0f, 1.0f, 1.0f, 1.0f

    };
    
    glBindVertexArray(FVAO);
    glBindBuffer(GL_ARRAY_BUFFER, FVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(fontVertices), fontVertices,
                      GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                               (void *)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, FEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
                      GL_DYNAMIC_DRAW);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
  }


    // glUniform2f(glGetUniformLocation(fontShaderProgram, "vertex"), )

  // glUseProgram(shaderProgram[1]);
  // glUniform1i(glGetUniformLocation(shaderProgram[1], "textureImg"), 2);
  // glBindVertexArray(VAO[1]);
  //   glUniform2f(glGetUniformLocation(shaderProgram[1], "offset"),
  //               0, 0);
  //   glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    
  // glfwSwapBuffers(window);
  // glfwPollEvents();
}

void drawOnce(GLFWwindow *window) {
  // glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
  // glClear(GL_COLOR_BUFFER_BIT);

  glUseProgram(shaderProgram[0]);
  glBindVertexArray(VAO[0]);

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

  glBindVertexArray(bVAO);
  glUniform2f(glGetUniformLocation(shaderProgram[0], "offset"), 0, 0);
  glUniform3f(glGetUniformLocation(shaderProgram[0], "textureColor"), 0, 0, 0);
  glDrawArrays(GL_TRIANGLES, 0, 12);

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
    glBindVertexArray(VAO[1]);
    glUniform2f(glGetUniformLocation(shaderProgram[0], "offset"),
                memOffset[i][0], memOffset[i][1]);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glUseProgram(0);
  }

}

//键盘按键回调函数  
void processInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, 1);
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
    bdy -= 0.001;
  }
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
    bdy += 0.001;
  }
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
    bdx += 0.001;
  }
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
    bdx -= 0.001;
  }
  // printf("%f %f\n", bdx, bdy);
}

// 鼠标回调函数
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {}

void mouse_click_callback(GLFWwindow *window, int button, int action,
                          int mods) {
  double cx, cy;
  if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
    glfwGetCursorPos(window, &cx, &cy);

    for (int i = 0; i < N; i++) {
      for (int j = 0; j < 4; j++) {
        if (dotInBox(cx * 2 / WW - 1 - pcOffset[i][j][0],
                     1 - cy * 2 / WH - pcOffset[i][j][1], pcVertices[0],
                     pcVertices[1], pcVertices[8], pcVertices[9])) {
          // printf("Box: (%d, %d)\n", i, j);
          Selected.type = j;
          Selected.id = i;
          Selected.work_time = &workTime[i][j];
          Selected.free_time = &freeTime[i][j];
          goto click_mem;
        }
      }
    }

    // mem
  click_mem:
    for (int i = 0; i < N; i++) {
      if (dotInBox(cx * 2 / WW - 1 - memOffset[i][0],
                   1 - cy * 2 / WH - memOffset[i][1], memVertices[0],
                   memVertices[1], memVertices[8], memVertices[9])) {
        // printf("memBox: %d\n", i);
        Selected.type = 4;
        Selected.id = i;
        break;
      }
    }

    if (Selected.type == 4)
      return;
    // button
    for (int i = 0; i < 4; i++) {
      if (dotInTriangle(cx / WW * 2 - 1, 1 - cy * 2 / WH, buttonVertices[i * 12 + 0],
                       buttonVertices[i * 12 + 1], buttonVertices[i * 12 + 4],
                       buttonVertices[i * 12 + 5], buttonVertices[i * 12 + 8],
                       buttonVertices[i * 12 + 9])) {
        printf("in tra %d\n", i);
        if (i == 0) {
        }
        break;
      }
    }
    // printf("click %lf %lf\n", cx, cy);
    // printf("click %lf %lf %d\n", cx, cy,
    //        dotInTriangle(cx / WW - 1, 1 - cy / WH, float x1, float y1, float x2, float y2, float x3, float y3)
    //        );
  }
}

void *view(void *arg) {
  view_data();

  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  stbi_set_flip_vertically_on_load(1);

  GLFWwindow *window = glfwCreateWindow(WW, WH, "Apple-Orange Problem 1.0", NULL, NULL);
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
  glGetShaderiv(fontVertexShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(fontVertexShader, 512, NULL, infoLog);
    printf("font vertex shader compile fail\n");
  }

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

  /*******字体着色器*******/
  const char *fontFragmentShaderSource =
      getShaderSource("../shader/fontFragmentShader.glsl");
  GLuint fontFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fontFragmentShader, 1, &fontFragmentShaderSource, NULL);
  glCompileShader(fontFragmentShader);
  glGetShaderiv(fontFragmentShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(fontFragmentShader, 512, NULL, infoLog);
    printf("font fragment shader compile fail\n");
  }

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
  glGetProgramiv(fontShaderProgram, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(fontShaderProgram, 512, NULL, infoLog);
    printf("font shader program link fail\n");
  }

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

  glGenVertexArrays(1, &bVAO);
  glGenBuffers(1, &bVBO);
  glBindVertexArray(bVAO);
  glBindBuffer(GL_ARRAY_BUFFER, bVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(buttonVertices), buttonVertices,
               GL_DYNAMIC_DRAW);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                        (void *)(2 * sizeof(float)));
  glEnableVertexAttribArray(1);
  // 纹理
  GLuint texture[2];
  glGenTextures(2, texture);
  genTextureFromFile(GL_TEXTURE0, texture[0], "../img/apple.jpg");
  genTextureFromFile(GL_TEXTURE1, texture[1], "../img/orange.jpg");

  // glEnable(GL_DEPTH_TEST);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  
  fread(fontBuffer, 1, 1 << 20, fopen("../ttf/CascadiaCode.ttf", "rb"));
  // Setting some state
  // glDisable(GL_CULL_FACE);
  // glDepthMask(GL_TRUE);
  // glEnable(GL_DEPTH_TEST);
  // glDepthFunc(GL_LEQUAL);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  // glPolygonMode(GL_FRONT, GL_LINE);
  // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  // glfwSetCursorPosCallback(window, mouse_callback); // 设置鼠标移动回调参数
  // 鼠标点击时间回调
  glfwSetMouseButtonCallback(window, mouse_click_callback);

  float ds0 = 0.054f, ds1 = 0.235f;
  float s0 = 0.093f, s1 = 0.114f;
  float z0 = 0.082f;

  // bdx = s0;
  // bdy = z0;
  // dotInTriangle(0.5, 0.5, 0, 0, 1, 0, 0, 1);

  while (!glfwWindowShouldClose(window)) {
    glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // s0 = bdx;
    // z0 = bdy;
    
    processInput(window);
    drawOnce(window);

    // drawFont(window, s0, s1, "Apple", 40);
    // drawFont(window, z0, s1 + ds0, "Producer", 40);
    // drawFont(window, s0, s1 + ds1, "Orange", 40);
    // drawFont(window, z0, s1 + ds1 + ds0, "Producer", 40);
    // drawFont(window, s0, s1 + 2 * ds1, "Apple", 40);
    // drawFont(window, z0, s1 + 2 * ds1 + ds0, "Comsumer", 40);
    // drawFont(window, s0, s1 + 3 * ds1, "Orange", 40);
    // drawFont(window, z0, s1 + 3 * ds1 + ds0, "Consumer", 40);
    // drawFont(window, 0.090, 1.450, "Memories", 40);

    glfwSwapBuffers(window);
    glfwPollEvents();
    usleep(33000);
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
