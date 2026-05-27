#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;

out vec3 vColor;

// Transform matrix
uniform mat4 uModel;

// Camera matrices
uniform mat4 uView;
uniform mat4 uProj;

void main()
{
    vColor = color;
    // mvp transformation
    gl_Position = uProj * uView * uModel * vec4(position, 1.0);
}