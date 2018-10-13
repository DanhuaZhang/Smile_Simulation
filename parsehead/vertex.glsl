#version 150 core

in vec3 position;

uniform vec3 inColor;

out vec3 Color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

void main() {
   Color = inColor;
   gl_Position = proj * view * model * vec4(position,1.0);
}