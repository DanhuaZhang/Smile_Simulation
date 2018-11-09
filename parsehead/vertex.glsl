#version 150 core

in vec4 position;

uniform vec3 inColor;

uniform uint color_flag;

out vec3 Color;

void main() {
    bool green_flag = bool(color_flag & uint( 1 << gl_VertexID ));
    Color = green_flag ? vec3( 1.0, 0.0, 0.0 ) : inColor;
    gl_Position = position;
}