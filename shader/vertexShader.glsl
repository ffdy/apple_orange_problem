#version 420 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;

uniform vec2 offset;

out vec2 TexCoord;
void main() {
  gl_Position = vec4(aPos + offset, 0.0, 1.0);
  TexCoord = aTexCoord;
}