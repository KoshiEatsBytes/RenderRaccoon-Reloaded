#version 330 core

out vec4 FragColor;

in      vec2  vUV;
flat in float vLayer;
flat in float vShade;

uniform sampler2DArray uBlockTex;
uniform vec3 uTint[32]; // size = tex::count

void main()
{
    // round vlayer to nearest slice
    vec4 tex  = texture(uBlockTex, vec3(vUV, vLayer));
    vec3 tint = uTint[int(vLayer)];

    FragColor = vec4(tex.rgb * tint * vShade, 1.0);
}
