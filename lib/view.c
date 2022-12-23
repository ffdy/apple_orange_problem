#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include "data.h" // 共享数据
#include "view.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" // 图片数据导入

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h" // 字体数据导入

#define FPS 30  // 显示帧率
#define WW 1600 // 显示宽度
#define WH 900  // 显示高度

pthread_t view_thread;
void view_start() { pthread_create(&view_thread, NULL, view, NULL); }
void view_done() { pthread_join(view_thread, NULL); }

float producer_consumer_vertices[] = {
    //-----坐标---- ---纹理坐标---
    -0.741f, 0.926, 0.0f, 0.0f,
    -0.741f, 0.838, 0.0f, 1.0f, 
    -0.634f, 0.838, 1.0f, 1.0f,
    -0.634f, 0.926f, 1.0f, 0.0f
};

float mem_vertices[] = {
    -0.727f, -0.147f, 0.0f, 0.0f,
    -0.727f, -0.368f, 0.0f, 1.0f,
    -0.591f, -0.368f, 1.0f, 1.0f,
    -0.591f, -0.147f, 1.0f, 0.0f
};

float button_vertices[] = {
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

unsigned int indices[] = {0, 1, 3, 1, 2, 3};

float texture_color[3][3] = {
    0.5f, 0.5f, 0.5f,
    1.0f, 1.0f, 0.0f,
    0.0f, 0.5f, 0.0f
};

float producer_consumer_offset[N][4][2];
float mem_offset[N + 1][2]; // 多一个Info底下选中mem时的显示

float producer_consumer_dw = 0.125f, producer_consumer_dh = 0.118f;
float mem_dw = 0.173f, mem_dh = 0.397f;

struct Info { // 选中的区域信息
  int id, type; // type：0123分别表示生产消费者，4为mem；id：对应的编号
} info_select;

GLuint VAO[2], VBO[2], EBO[2];
GLuint bVAO, bVBO;
GLuint shader_program[3];

GLuint font_shader_program;

GLubyte ttf_file_buffer[1 << 20];
GLubyte temp_bitmap[1 << 20];
GLubyte rgba_data[1 << 22];
stbtt_bakedchar char_data[96];

/**
 * @brief 初始化图形化所需部分的参数
 * 
 */
void view_data() {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < N; j += 2) {
      producer_consumer_offset[j][i][0] = (int)(j / 2) * producer_consumer_dw;
      producer_consumer_offset[j + 1][i][0] =
          (int)(j / 2) * producer_consumer_dw;
      producer_consumer_offset[j][i][1] = 0 - 2 * i * producer_consumer_dh;
      producer_consumer_offset[j + 1][i][1] =
          0 - (2 * i + 1) * producer_consumer_dh;
    }
  }
  for (int i = 0; i < N; i += 2) {
    mem_offset[i][0] = (int)(i / 2) * mem_dw;
    mem_offset[i][1] = 0;
    mem_offset[i + 1][0] = (int)(i / 2) * mem_dw;
    mem_offset[i + 1][1] = -mem_dh;
  }

  mem_offset[N][0] = 1.428f; // Info区域的offset
  mem_offset[N][1] = 0.534f;

  fread(ttf_file_buffer, 1, 1 << 20, fopen("font/CascadiaCode.ttf", "rb"));
}

/**
 * @brief 判断给定点是否在指定三角形中（面积法）
 * 
 * @param x 给定点横坐标
 * @param y 给定点纵坐标
 * @param x1 第一个点横坐标
 * @param y1 第一个点纵坐标
 * @param x2 
 * @param y2 
 * @param x3 
 * @param y3 
 * @return int 1 在
 * @return int 0 不在
 */
int dot_in_triangle(float x, float y, float x1, float y1, float x2, float y2,
                    float x3, float y3) {
  float s = fabsf((x2 - x1) * (y3 - y2) - (x3 - x1) * (y2 - y1)) / 2;
  float s1 = fabsf((x1 - x) * (y2 - y) - (x2 - x) * (y1 - y)) / 2;
  float s2 = fabsf((x1 - x) * (y3 - y) - (x3 - x) * (y1 - y)) / 2;
  float s3 = fabsf((x3 - x) * (y2 - y) - (x2 - x) * (y3 - y)) / 2;

  if (fabsf(s1 + s2 + s3 - s) < 0.01)
    return 1;
  return 0;
}

/**
 * @brief 给定点是否在给定正放四边形中
 * 
 * @param x 给定点横坐标
 * @param y 给定点纵坐标
 * @param x1 x左边界
 * @param y1 y上边界
 * @param x2 x右边界
 * @param y2 y下边界
 * @return int 1 在
 * @return int 0 不在
 */
int dot_in_box(float x, float y, float x1, float y1, float x2, float y2) {
  if (x <= x2 && x >= x1 && y >= y2 && y <= y1)
    return 1;
  return 0;
}

/**
 * @brief Get the shader source object
 * 使用之后需要将指向的空间释放
 * @param path glsl文件路径
 * @return char* 指向读取的glsl代码串
 */
char *get_shader_source(const char *path) {
  FILE *file = fopen(path, "r");
  fseek(file, 0, SEEK_END);
  int file_size = ftell(file) + 1;
  rewind(file);

  char *shader_source = malloc(file_size);
  file_size = fread(shader_source, 1, file_size, file);
  fclose(file);
  shader_source[file_size] = '\0';
  return shader_source;
}

/**
 * @brief 从图片文件中生成纹理
 * 
 * @param texture_id 纹理标识
 * @param texture_place 绑定的纹理号
 * @param img_path 待绑定的图片路径
 */
void gen_texture_from_file(GLuint texture_id, GLuint texture_place,
                           const char *img_path) {
  glActiveTexture(texture_id);
  glBindTexture(GL_TEXTURE_2D, texture_place);
  // 设定对象环绕、过滤方式
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  // 加载图片
  int img_width, img_height, channels;
  unsigned char *img_data =
      stbi_load(img_path, &img_width, &img_height, &channels, 0);
  if (img_data) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img_width, img_height, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, img_data);
    glGenerateMipmap(GL_TEXTURE_2D); // 自动生成多级渐远纹理
  } else {
    printf("image load fail;\n");
  }
  stbi_image_free(img_data);
}

/**
 * @brief 从ttf_file_buffer中生成指定高度的字体纹理
 * 需要提前将字体文件读入ttf_file_buffer中
 * @param texture_id 纹理标识
 * @param texture_place 绑定的纹理号
 * @param font_height 需要生成的字体高度
 */
void gen_texture_from_ttf(GLuint texture_id, GLuint texture_place,
                          GLfloat font_height) {

  stbtt_BakeFontBitmap(ttf_file_buffer, 0, font_height, temp_bitmap, 1024, 1024,
                       32, 96, char_data);
  memset(rgba_data, 0, sizeof rgba_data);

  for (int i = 0; i < 1024 * 1024; i++) {
    rgba_data[4 * i + 3] = temp_bitmap[i];
  }

  glActiveTexture(texture_id);
  glBindTexture(GL_TEXTURE_2D, texture_place);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1024, 1024, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, rgba_data);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

/**
 * @brief 从指定的着色器代码中编译生成着色器
 * 
 * @param glsl_path 着色器代码文件路径
 * @param shader_type 着色器类型（顶点/片段）
 * @return GLint 着色器标识
 */
GLint gen_shader(const char *glsl_path, GLint shader_type) {
  GLuint shader_id = glCreateShader(shader_type);

  int success;
  char info_log[512];
  const char *shader_source = get_shader_source(glsl_path);
  glShaderSource(shader_id, 1, &shader_source, NULL);
  glCompileShader(shader_id);
  glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(shader_id, 512, NULL, info_log);
    printf("shader compile fail\n");
  }

  free((char *)shader_source);
  return shader_id;
}

/**
 * @brief 生成特定的着色器程序
 * 
 * @param vertex_glsl_path 顶点着色器代码路径
 * @param fragment_glsl_path 片段着色器代码路径
 * @return GLint 着色器程序标识
 */
GLint gen_shader_program(char *vertex_glsl_path, char *fragment_glsl_path) {
  GLint shader_program = glCreateProgram();
  GLint vertex_shader = gen_shader(vertex_glsl_path, GL_VERTEX_SHADER);
  GLint fragment_shader = gen_shader(fragment_glsl_path, GL_FRAGMENT_SHADER);

  glAttachShader(shader_program, vertex_shader);
  glAttachShader(shader_program, fragment_shader);
  glLinkProgram(shader_program);

  int success;
  char info_log[512];
  glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(shader_program, 512, NULL, info_log);
    printf("shader program link fail\n");
  }

  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  return shader_program;
}

/**
 * @brief 生成VAO，VBO，EBO并绑定数据
 * 
 * @param VAO_s VAO标识
 * @param VBO_s VBO标识
 * @param EBO_s EBO标识，NULL表示无需EBO
 * @param VBO_data 待绑定VBO的数据
 * @param VBO_data_size 数据大小
 * @param EBO_data 待绑定EBO的数据
 * @param EBO_data_size 数据大小
 */
void gen_vertex_arrays(GLuint *VAO_s, GLuint *VBO_s, GLuint *EBO_s,
                       float *VBO_data, int VBO_data_size, GLuint *EBO_data,
                       int EBO_data_size) {

  glGenVertexArrays(1, VAO_s);
  glGenBuffers(1, VBO_s);

  glBindVertexArray(*VAO_s);
  glBindBuffer(GL_ARRAY_BUFFER, *VBO_s);
  glBufferData(GL_ARRAY_BUFFER, VBO_data_size, VBO_data, GL_DYNAMIC_DRAW);

  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                        (void *)(2 * sizeof(float)));
  glEnableVertexAttribArray(1);

  if (EBO_s == NULL)  // 不需要绑定EBO
    return;
  glGenBuffers(1, EBO_s);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *EBO_s);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, EBO_data_size, EBO_data,
               GL_DYNAMIC_DRAW);
}

GLuint font_texture;
GLuint FVAO, FVBO, FEBO;
/**
 * @brief 生成字体纹理相关的VAO，VBO，EBO并绑定部分数据
 * 
 */
void gen_text_vertex_arrays() {
  // float font_vertices[16];
  glGenVertexArrays(1, &FVAO);
  glGenBuffers(1, &FVBO);
  glGenBuffers(1, &FEBO);

  glBindVertexArray(FVAO);
  glBindBuffer(GL_ARRAY_BUFFER, FVBO);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, FEBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
               GL_DYNAMIC_DRAW);
}

/**
 * @brief 在指定位置渲染给定的文本
 * 文字坐标系
 * +---->x
 * |
 * |y
 * @param x 左上角点x坐标
 * @param y 左上角点y坐标
 * @param msg 需要渲染文本字符指针
 * @param font_height 需要渲染文本的高度
 */
void draw_text(float x, float y, char *msg,
               float font_height) {
  glGenTextures(1, &font_texture);

  x *= WW;
  y *= WH;

  glUseProgram(font_shader_program);
  glBindVertexArray(FVAO);
  glUniform1i(glGetUniformLocation(font_shader_program, "texImg"), 2);

  gen_texture_from_ttf(GL_TEXTURE2, font_texture, font_height);

  stbtt_aligned_quad q;
  for (char *c = msg; *c != '\0'; c++) {
    stbtt_GetBakedQuad(char_data, 1024, 1024, *c - 32, &x, &y, &q, 1);

    float font_vertices[] = {
        (q.x0 - WW) / WW, (-q.y0 - font_height + WH) / WH, q.s0, q.t0,
        (q.x0 - WW) / WW, (-q.y1 - font_height + WH) / WH, q.s0, q.t1,
        (q.x1 - WW) / WW, (-q.y1 - font_height + WH) / WH, q.s1, q.t1,
        (q.x1 - WW) / WW, (-q.y0 - font_height + WH) / WH, q.s1, q.t0};
    glBufferData(GL_ARRAY_BUFFER, sizeof(font_vertices), font_vertices,
                 GL_DYNAMIC_DRAW);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
  }

  glDeleteTextures(1, &font_texture);
}
/**
 * @brief 主要的渲染任务
 * 
 */
void draw_textures() {

  glUseProgram(shader_program[0]);
  glBindVertexArray(VAO[0]);

  for (int i = 0; i < N; i++) {
    for (int j = 0; j < 4; j++) {
      glUniform2f(glGetUniformLocation(shader_program[0], "offset"),
                  producer_consumer_offset[i][j][0],
                  producer_consumer_offset[i][j][1]);
      glUniform3f(glGetUniformLocation(shader_program[0], "textureColor"),
                  texture_color[producer_consumer_state[i][j]][0],
                  texture_color[producer_consumer_state[i][j]][1],
                  texture_color[producer_consumer_state[i][j]][2]);
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }
  }

  int mem_draw_times = N;

  if (info_select.type != MEMORY) {  // 非mem显示调整速度的按钮
    glBindVertexArray(bVAO);
    glUniform2f(glGetUniformLocation(shader_program[0], "offset"), 0, 0);
    glUniform3f(glGetUniformLocation(shader_program[0], "textureColor"), 0, 0,
                0);
    glDrawArrays(GL_TRIANGLES, 0, 12);
  } else {  // mem则将按钮区域渲染成选中mem的状态
    mem_draw_times = N + 1;
  }

  for (int i = 0; i < mem_draw_times; i++) {
    int mem_id = i;
    if (i == N) {
      mem_id = info_select.id;
    }
    if (mem_state[mem_id] == MEM_FREE) {
      glUseProgram(shader_program[0]);
      glUniform3f(glGetUniformLocation(shader_program[0], "textureColor"),
                  texture_color[0][0], texture_color[0][1],
                  texture_color[0][2]);
    } else if (mem_state[mem_id] == MEM_APPLE_PRODUCE) {
      glUseProgram(shader_program[1]);
      glUniform1i(glGetUniformLocation(shader_program[1], "textureImg"), 0);
    } else if (mem_state[mem_id] == MEM_APPLE_WAITING || mem_state[mem_id] == MEM_APPLE_CONSUME) {
      glUseProgram(shader_program[2]);
      glUniform1i(glGetUniformLocation(shader_program[2], "textureImg"), 0);
    } else if (mem_state[mem_id] == MEM_ORANGE_PRODUCE) {
      glUseProgram(shader_program[1]);
      glUniform1i(glGetUniformLocation(shader_program[1], "textureImg"), 1);
    } else if (mem_state[mem_id] == MEM_ORANGE_WAITING || mem_state[mem_id] == MEM_ORANGE_CONSUME) {
      glUseProgram(shader_program[2]);
      glUniform1i(glGetUniformLocation(shader_program[2], "textureImg"), 1);
    }
    glBindVertexArray(VAO[1]);
    glUniform2f(glGetUniformLocation(shader_program[0], "offset"),
                mem_offset[i][0], mem_offset[i][1]);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glUseProgram(0);
  }
}

/**
 * @brief 键盘按键回调函数
 * 
 * @param window 窗口标识
 */
void keyboard_callback(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, 1);
}

/**
 * @brief 鼠标点击回调函数，用于按钮的交互和选择对应的对象
 * 
 * @param window 窗口标识
 * @param button 
 * @param action 
 * @param mods 
 */
void mouse_click_callback(GLFWwindow *window, int button, int action,
                          int mods) {
  double cx, cy;
  if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
    glfwGetCursorPos(window, &cx, &cy);   //捕获左键单击坐标

    for (int i = 0; i < N; i++) {
      for (int j = 0; j < 4; j++) {
        if (dot_in_box(
                cx * 2 / WW - 1 - producer_consumer_offset[i][j][0],
                1 - cy * 2 / WH - producer_consumer_offset[i][j][1],
                producer_consumer_vertices[0], producer_consumer_vertices[1],
                producer_consumer_vertices[8], producer_consumer_vertices[9])) {
          info_select.type = j;
          info_select.id = i;
          goto click_mem;
        }
      }
    }

    // 选中mem
  click_mem:
    for (int i = 0; i < N; i++) {
      if (dot_in_box(cx * 2 / WW - 1 - mem_offset[i][0],
                     1 - cy * 2 / WH - mem_offset[i][1], mem_vertices[0],
                     mem_vertices[1], mem_vertices[8], mem_vertices[9])) {
        info_select.type = 4;
        info_select.id = i;
        break;
      }
    }

    if (info_select.type == 4)
      return;
    // 点击button
    for (int i = 0; i < 4; i++) {
      if (dot_in_triangle(
              cx / WW * 2 - 1, 1 - cy * 2 / WH, button_vertices[i * 12 + 0],
              button_vertices[i * 12 + 1], button_vertices[i * 12 + 4],
              button_vertices[i * 12 + 5], button_vertices[i * 12 + 8],
              button_vertices[i * 12 + 9])) {
        printf("in tra %d\n", i);
        if (i == 0) {
          if (free_time[info_select.id][info_select.type] == 0)
            return;
          free_time[info_select.id][info_select.type]--;
        } else if (i == 1) {
          free_time[info_select.id][info_select.type]++;
        } else if (i == 2) {
          if (work_time[info_select.id][info_select.type] == 0)
            return;
          work_time[info_select.id][info_select.type]--;
        } else {
          work_time[info_select.id][info_select.type]++;
        }
        break;
      }
    }
  }
}

/**
 * @brief 视图线程的任务函数
 * 
 * @param arg 
 * @return void* 
 */
void *view(void *arg) {
  view_data();

  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  // stbi_set_flip_vertically_on_load(1);

  GLFWwindow *window =
      glfwCreateWindow(WW, WH, "Apple-Orange Problem 2.0", NULL, NULL);
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

  // 编译链接着色器程序
  font_shader_program =
      gen_shader_program("shader/font_vertex_shader.glsl",
                         "shader/font_fragment_shader.glsl");
  shader_program[0] = gen_shader_program(
      "shader/vertex_shader.glsl", "shader/fragment_shader_color.glsl");
  shader_program[1] = gen_shader_program("shader/vertex_shader.glsl",
                                         "shader/fragment_shader_gray.glsl");
  shader_program[2] = gen_shader_program(
      "shader/vertex_shader.glsl", "shader/fragment_shader_texture.glsl");

  // VAO，VBO，EBO绑定
  gen_vertex_arrays(&VAO[0], &VBO[0], &EBO[0], producer_consumer_vertices,
                    sizeof(producer_consumer_vertices), indices,
                    sizeof(indices));
  gen_vertex_arrays(&VAO[1], &VBO[1], &EBO[1], mem_vertices,
                    sizeof(mem_vertices), indices, sizeof(indices));
  gen_vertex_arrays(&bVAO, &bVBO, NULL, button_vertices,
                    sizeof(button_vertices), NULL, 0);
  gen_text_vertex_arrays();

  // 纹理
  GLuint texture[2];
  glGenTextures(2, texture);
  gen_texture_from_file(GL_TEXTURE0, texture[0], "img/apple.jpg");
  gen_texture_from_file(GL_TEXTURE1, texture[1], "img/orange.jpg");

  glEnable(GL_BLEND); // 启用alpha通道混合（文本纹理）
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glfwSetMouseButtonCallback(window,
                             mouse_click_callback); // 注册鼠标点击时回调函数

  float ds0 = 0.054f, ds1 = 0.235f; // 相关文本的偏移量辅助数据
  float s0 = 0.093f, s1 = 0.114f;
  float z0 = 0.082f;
  float ts0 = 1.756f, ts1 = 0.810f, ts2 = 0.572f;
  float tz0 = 0.013f, tz1 = 0.047f, tz2 = -0.023f, tz3 = -0.784f;
  float tz4 = -0.168f, tz5 = -0.555f;

  char cbuf[100]; // 文本打印缓冲

  while (!glfwWindowShouldClose(window)) {
    glClearColor(1.0f, 1.0f, 1.0f, 0.0f); // 清空图形区域
    glClear(GL_COLOR_BUFFER_BIT);

    keyboard_callback(window); // 处理键盘输入
    draw_textures();     // 绘制区域及其纹理

    // 分块的标识
    draw_text(s0, s1, "Apple", 40);
    draw_text(z0, s1 + ds0, "Producer", 40);
    draw_text(s0, s1 + ds1, "Orange", 40);
    draw_text(z0, s1 + ds1 + ds0, "Producer", 40);
    draw_text(s0, s1 + 2 * ds1, "Apple", 40);
    draw_text(z0, s1 + 2 * ds1 + ds0, "Comsumer", 40);
    draw_text(s0, s1 + 3 * ds1, "Orange", 40);
    draw_text(z0, s1 + 3 * ds1 + ds0, "Consumer", 40);
    draw_text(0.090, 1.450, "Memories", 40);

    draw_text(ts0 + tz2, ts1 + tz3, "Info", 70);

    // 选中的对象
    draw_text(ts0 + tz4, ts1 + tz5 - 0.08f, " Select : ", 40);
    if (info_select.type == 0) {
      sprintf(cbuf, "Apple Producer %d", info_select.id + 1);
    } else if (info_select.type == 1) {
      sprintf(cbuf, "Orange Producer %d", info_select.id + 1);
    } else if (info_select.type == 2) {
      sprintf(cbuf, "Apple Consumer %d", info_select.id + 1);
    } else if (info_select.type == 3) {
      sprintf(cbuf, "Orange Consumer %d", info_select.id + 1);
    } else {
      sprintf(cbuf, "Memory %d", info_select.id + 1);
    }
    draw_text(ts0 + tz4 + 0.12f, ts1 + tz5 - 0.08f, cbuf, 40);

    // 显示选中对象的状态
    draw_text(ts0 + tz4, ts1 + tz5, " Status : ", 40);
    if (info_select.type == 4) {
      if (mem_state[info_select.id] == 0) {
        sprintf(cbuf, "Free");
      } else if (mem_state[info_select.id] == 1) {
        sprintf(cbuf, "Producer %d", mem_host[info_select.id] + 1);
        draw_text(ts0 + tz4 + 0.12f, ts1 + tz5 + 0.08f, cbuf, 40);
        sprintf(cbuf, "Occupied by Apple");
      } else if (mem_state[info_select.id] == 2) {
        sprintf(cbuf, "Occupied by Apple");
      } else if (mem_state[info_select.id] == 3) {
        sprintf(cbuf, "Producer %d", mem_host[info_select.id] + 1);
        draw_text(ts0 + tz4 + 0.12f, ts1 + tz5 + 0.08f, cbuf, 40);
        sprintf(cbuf, "Occupied by Orange");
      } else if (mem_state[info_select.id] == 4) {
        sprintf(cbuf, "Occupied by Orange");
      } else if (mem_state[info_select.id] == 5) {
        sprintf(cbuf, "Consumer %d", mem_host[info_select.id] + 1);
        draw_text(ts0 + tz4 + 0.12f, ts1 + tz5 + 0.08f, cbuf, 40);
        sprintf(cbuf, "Occupied by Apple");
      } else {
        sprintf(cbuf, "Consumer %d", mem_host[info_select.id] + 1);
        draw_text(ts0 + tz4 + 0.12f, ts1 + tz5 + 0.08f, cbuf, 40);
        sprintf(cbuf, "Occupied by Orange");
      }
    } else {
      if (producer_consumer_state[info_select.id][info_select.type] == FREE) {
        sprintf(cbuf, "Free");
      } else if (producer_consumer_state[info_select.id][info_select.type] ==
                 WAITING) {
        sprintf(cbuf, "Waiting for Memory");
      } else if (producer_consumer_state[info_select.id][info_select.type] ==
                 PRODUCING) {
        if (info_select.type == APPLE_PRODUCER || info_select.type == ORANGE_PRODUCER) {
          sprintf(cbuf, "Producing at Memory %d",
                  producer_consumer_target[info_select.id][info_select.type] +
                      1);
        } else {
          sprintf(cbuf, "Consuming at Memory %d",
                  producer_consumer_target[info_select.id][info_select.type] +
                      1);
        }
      }
    }
    draw_text(ts0 + tz4 + 0.12f, ts1 + tz5, cbuf, 40);

    if (info_select.type != 4) { // 非选中mem时显示调整速度的按钮
      draw_text(ts0, ts1, "Work", 40);
      draw_text(ts0, ts2, "Free", 40);

      sprintf(cbuf, "%ds", work_time[info_select.id][info_select.type]);
      draw_text(ts0, ts1 + tz1, cbuf, 60);
      sprintf(cbuf, "%ds", free_time[info_select.id][info_select.type]);
      draw_text(ts0, ts2 + tz1, cbuf, 60);
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
    usleep(1e6 / FPS); // 降低帧率
  }

  // 回收OpenGL占用的资源
  glDeleteVertexArrays(2, VAO);
  glDeleteVertexArrays(1, &bVAO);
  glDeleteVertexArrays(1, &FVAO);
  glDeleteBuffers(2, VBO);
  glDeleteBuffers(1, &bVBO);
  glDeleteBuffers(1, &FVBO);
  glDeleteBuffers(2, EBO);
  glDeleteBuffers(1, &FEBO);
  glDeleteTextures(2, texture);

  for (int i = 0; i < 3; i++) {
    glDeleteProgram(shader_program[i]);
  }
  glDeleteProgram(font_shader_program);

  glfwTerminate();
  return NULL;
}