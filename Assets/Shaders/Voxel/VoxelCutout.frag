#version 330 core

out vec4 FragColor;

in      vec2  vUV;
flat in float vLayer;
flat in float vShade;

uniform sampler2DArray uBlockTex;
uniform vec3 uTint[96];

void main()
{
    vec4 tex = texture(uBlockTex, vec3(vUV, vLayer));
    // drop transparent panels
    if (tex.a < 0.5) discard;

    vec3 tint = uTint[int(vLayer)];
    FragColor = vec4(tex.rgb * tint * vShade, 1.0);
}