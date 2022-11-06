#version 330 core
out vec4 color;

uniform sampler2D text;
uniform vec2 TexCoords;
// uniform vec3 textColor;

void main() {
  vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
  color = vec4(0.0, 0.0, 0.0, 1.0) * sampled;
}