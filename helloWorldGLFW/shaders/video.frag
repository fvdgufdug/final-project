#version 330 core
in vec2 vUV;
uniform sampler2D uTex;
out vec4 FragColor;
void main() { FragColor = vec4(texture(uTex, vUV).rgb, 1.0); }
