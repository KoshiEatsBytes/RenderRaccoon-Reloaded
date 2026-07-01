#version 330 core

layout (location = 0) in vec2 aPos;

out vec4 vWorld;

uniform mat4 uView;
uniform mat4 uProj;

void main()
{
    gl_Position = vec4(aPos, 1.0, 1.0);

    vWorld = inverse(uProj * uView) * vec4(aPos, 1.0, 1.0);
}
