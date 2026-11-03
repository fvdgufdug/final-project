#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;
uniform mat4 uOrtho;
uniform float uScale;
out vec4 vColor;
void main()
{
    vColor = aColor;
    gl_Position = uOrtho * vec4(aPos.xy * uScale, 0.0, 1.0);
}
