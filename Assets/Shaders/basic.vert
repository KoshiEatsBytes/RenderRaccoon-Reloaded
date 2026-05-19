#version 330 core

// Inputs
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;

uniform vec2 uOffset;

// Outputs
out vec3 vColor;

void main()
{
    vColor = color;
    gl_Position = vec4(position.x + uOffset.x, position.y + uOffset.y, position.z, 1.0);
}