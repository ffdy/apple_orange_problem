#version 420 core
in vec2 TexCoord;
out vec4 FragColor;
vec4 temp;

uniform sampler2D textureImg;

void main() {
  temp = texture(textureImg, TexCoord);
  float result = dot(temp.rgb, vec3(0.2125, 0.7154, 0.0721));
  FragColor = vec4(vec3(result), 1.0);
  // FragColor = temp;
}