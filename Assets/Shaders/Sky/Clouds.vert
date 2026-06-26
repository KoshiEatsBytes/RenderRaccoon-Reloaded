#version 330 core

layout (location = 0) in vec3  aPos;
layout (location = 1) in float aShade;

out float vShade;
out vec2  vWorldXZ;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

void main()
{
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vShade   = aShade;
    vWorldXZ = worldPos.xz;

    gl_Position = uProj * uView * worldPos;
}
