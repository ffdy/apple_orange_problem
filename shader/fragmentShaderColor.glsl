#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform vec3 textureColor;

void main() {
  FragColor = vec4(textureColor, 1.0);
}
