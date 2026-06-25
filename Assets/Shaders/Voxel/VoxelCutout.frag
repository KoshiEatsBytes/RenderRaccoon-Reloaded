#version 330 core

out vec4 FragColor;

in vec2  vUV;
in float vViewDist;

flat in float vLayer;
flat in float vShade;

uniform vec3  uFogColor;
uniform float uFogStart;
uniform float uFogEnd;

uniform sampler2DArray uBlockTex;
uniform vec3 uTint[96];

void main()
{
    vec4 tex = texture(uBlockTex, vec3(vUV, vLayer));
    // drop transparent panels
    if (tex.a < 0.5) discard;

    vec3 tint = uTint[int(vLayer)];

    vec3  lit = tex.rgb * tint * vShade;
    float fog = clamp((vViewDist - uFogStart) /
                     (uFogEnd - uFogStart), 0.0, 1.0);

    FragColor = vec4(mix(lit, uFogColor, fog), 1.0);
}