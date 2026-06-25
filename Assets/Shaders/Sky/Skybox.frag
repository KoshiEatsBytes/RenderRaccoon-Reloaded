#version 330 core

out vec4 FragColor;

in vec3 vDir;

uniform vec3 uSkyHorizon;
uniform vec3 uSkyZenith;

// skybox sun
uniform vec3      uSunDir;
uniform float     uSunSize;
uniform sampler2D uSunTex;

void main()
{
    // calcualte horizon zenith threshold and color
    vec3  dir = normalize(vDir);
    float t   = pow(clamp(dir.y, 0.0, 1.0), 0.5);
    vec3  col = mix(uSkyHorizon, uSkyZenith, t);

    // Render sun disk, only in one hemisphere
    if (dot(dir, uSunDir) > 0.0)
    {
        vec3 up = abs(uSunDir.y) > 0.99
                  ? vec3(1.0, 0.0, 0.0)
                  : vec3(0.0, 1.0, 0.0);

        vec3 right  = normalize(cross(up, uSunDir));
        vec3 realUp = cross(uSunDir, right);

        // map sun on skbox
        vec2 texUV = vec2(dot(dir, right), dot(dir, realUp)) / uSunSize;

        // place on disk, -1 to 1
        if(abs(texUV.x) <= 1.0 && abs(texUV.y) <= 1.0)
        {
            vec4 sun = texture(uSunTex, texUV * 0.5 + 0.5);
            col += sun.rgb;
        }
    }

    FragColor = vec4(col, 1.0);
}
