#version 330 core

struct Light
{
    vec3 color;
    vec3 position;
};

out vec4 FragColor;

in vec2 vUV;
in vec3 vNormal;
in vec3 vFragPos;

uniform sampler2D baseColorTexture;
uniform Light uLight;

void main()
{
    vec3 norm = normalize(vNormal);

    // calculate light direction
    vec3 lightDir = normalize(uLight.position - vFragPos);
    // calculate diffuse factor
    float diffFac = max(dot(norm, lightDir), 0.0);
    // calculate light diffuse
    vec3 diffuse = diffFac * uLight.color;

    vec4 texColor = texture(baseColorTexture, vUV);

    FragColor = texColor * vec4(diffuse, 1.0);
}