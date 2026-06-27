#version 330 core

out vec4 FragColor;

in float vShade;
in vec3  vWorldPos;

uniform vec3 uCameraPos;
uniform vec3 uHorizonColor;
uniform vec3 uZenithColor;
uniform vec3 uCloudColor;

uniform float uFogStart;
uniform float uFogEnd;

void main()
{
    // fade clouds to sky
    float dist = length(vWorldPos.xz - uCameraPos.xz);
    float fog  = clamp((dist - uFogStart) / (uFogEnd - uFogStart), 0.0, 1.0);

    // adapt fade to zenith gradient
    vec3 dir = normalize(vWorldPos - uCameraPos);
    float t  = pow(clamp(dir.y, 0.0, 1.0), 0.5);
    vec3 sky = mix(uHorizonColor, uZenithColor, t);

    vec3 col  = mix(uCloudColor * vShade, sky, fog);
    FragColor = vec4(col, 1.0);
}