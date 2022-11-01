#version 330 core
in vec2 TexCoord;
out vec4 FragColor;
vec4 temp;

uniform int state;
uniform sampler2D textureImg1;
uniform sampler2D textureImg2;

void main() {
  if (state == 0) {
    FragColor = vec4(0.8, 0.8, 0.8, 1.0);
  } else if (state == 1) {
    temp = texture(textureImg1, TexCoord);
    float result = dot(temp.rgb, vec3(0.2125, 0.7154, 0.0721));
    FragColor = vec4(vec3(result), 1.0);
  } else if (state == 2) {
    FragColor = texture(textureImg1, TexCoord);
  } else if (state == 3) {
    temp = texture(textureImg2, TexCoord);
    float result = dot(temp.rgb, vec3(0.2125, 0.7154, 0.0721));
    FragColor = vec4(vec3(result), 1.0);
  } else {
    FragColor = texture(textureImg2, TexCoord);
  }

  // FragColor = temp;
}
