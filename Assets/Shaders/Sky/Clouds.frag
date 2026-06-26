#version 330 core

out vec4 FragColor;

in float vShade;
in vec2  vWorldXZ;

uniform vec3  uCameraPos;
uniform vec3  uHorizonColor;
uniform vec3  uCloudColor;

uniform float uFogStart;
uniform float uFogEnd;

void main()
{
    // fade clouds to sky
    float dist = length(vWorldXZ - uCameraPos.xz);
    float fog  = clamp((dist - uFogStart) / (uFogEnd - uFogStart), 0.0, 1.0);
    vec3  col  = mix(uCloudColor * vShade, uHorizonColor, fog);

    FragColor  = vec4(col, 1.0);
}