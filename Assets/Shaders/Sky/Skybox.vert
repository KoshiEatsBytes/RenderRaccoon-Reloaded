#version 330 core

layout (location = 0) in vec2 aPos;

out vec3 vDir;

uniform mat4 uView;
uniform mat4 uProj;
uniform vec3 uCameraPos;

void main()
{
    gl_Position = vec4(aPos, 1.0, 1.0);

    mat4 invVP = inverse(uProj * uView);
    vec4 world = invVP * vec4(aPos, 1.0, 1.0);

    vDir = world.xyz / world.w - uCameraPos;
}
