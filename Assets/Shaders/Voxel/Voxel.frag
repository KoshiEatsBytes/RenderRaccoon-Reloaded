#version 330 core

out vec4 FragColor;

in vec2  vUV;
in float vViewDist;

flat in float vLayer;
flat in float vShade;

uniform vec3  uFogColor;
uniform float uFogStart;
uniform float uFogEnd;

uniform float uFlatStart;
uniform float uFlatEnd;

uniform sampler2DArray uBlockTex;
uniform vec3 uTint[96]; // same or more generous size of texture array
uniform vec3 uAvgColor[96]; // per layer avg

void main()
{
    int layer = int(vLayer);

    float flatten = clamp((vViewDist - uFlatStart) /
                          max(uFlatEnd - uFlatStart, 1.0), 0.0, 1.0);

    // color
    vec3 base = uAvgColor[layer];

    // flatten color if far away
    if (!(flatten >= 0.999))
    {
        base = mix(texture(uBlockTex, vec3(vUV, vLayer)).rgb,
                uAvgColor[layer], flatten);
    }

    vec3  lit = base * uTint[layer] * vShade;
    float fog = clamp((vViewDist - uFogStart) / (uFogEnd - uFogStart), 0.0, 1.0);

    FragColor = vec4(mix(lit, uFogColor, fog), 1.0);
}
