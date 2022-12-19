#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D texImg;
// uniform sampler2D texImg1;
// uniform vec3 textFragColor;

void main() {
  // vec4 sampled = vec4(1.0, 1.0, 1.0, texture(texImg, TexCoord).r);
  // FragColor = vec4(0.0, 0.0, 0.0, 1.0) * sampled;
  // FragColor = vec4(texture(texImg1, TexCoord).rgb, texture(texImg, TexCoord).a);
  // FragColor = vec4(1.0, 0, 0, texture(texImg, TexCoord).a);
  FragColor = texture(texImg, TexCoord);
  // FragColor = vec4(FragColor.r, 0, 0, 1.0);
  // FragColor = vec4(0,0,0,0.1);
}