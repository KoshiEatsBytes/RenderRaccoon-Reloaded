#version 330 core

layout (location = 0) in vec3  aPos;
layout (location = 1) in float aShade;

out float vShade;
out vec3  vWorldPos;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

void main()
{
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vShade   = aShade;
    vWorldPos = worldPos.xyz;

    gl_Position = uProj * uView * worldPos;
}
