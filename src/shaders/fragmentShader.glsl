#version 330 core
out vec4 FragColor;
uniform float speedSq;

void main()
{
    FragColor = vec4(1.0f, 0.0f, speedSq / (1000.0f + speedSq), 1.0f);
}