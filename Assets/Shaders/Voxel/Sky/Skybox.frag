#version 330 core

out vec4 FragColor;

in vec3 vDir;

uniform vec3 uSkyHorizon;
uniform vec3 uSkyZenith;

void main()
{
    // calcualte horizon zenith threshold
    vec3  dir = normalize(vDir);
    float t   = pow(clamp(dir.y, 0.0, 1.0), 0.5);

    FragColor = vec4(mix(uSkyHorizon, uSkyZenith, t), 1.0);
}
