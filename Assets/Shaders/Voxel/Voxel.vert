#version 330 core

// voxel stride map
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;
layout (location = 3) in float aLayer;

out vec2  vUV;
out float vViewDist;

flat out float vLayer; // flat stops interpolation
flat out float vShade;

// chunk/tile world translation is baked at pos upload
uniform mat4 uView;
uniform mat4 uProj;
uniform vec3 uCameraPos;

float faceShade(vec3 face)
{
    // Up
    if (face.y > 0.5) return 1.0;
    // Down
    if (face.y < -0.5) return 0.5;
    // North / south
    if (abs(face.z) > 0.5) return 0.8;
    // East West
    return 0.6;
}

void main()
{
    vUV = aUV;
    vLayer = aLayer;
    vShade = faceShade(aNormal);

    // aPos is already in world space
    vViewDist = length(aPos.xz - uCameraPos.xz);

    gl_Position = uProj * uView * vec4(aPos, 1.0);
}
