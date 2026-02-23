#version 330 core
layout (location = 0) in vec2 aPos;

uniform mat4 model;      // object transform
uniform mat4 view;       // camera view
uniform mat4 projection; // projection

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 0.0, 1.0);
}