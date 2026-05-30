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
    vec3 lightDir = normalize(uLight.position - vFragPos);
    float diffFac = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diffFac * uLight.color;

    vec4 texColor = texture(baseColorTexture, vUV);

    FragColor = texColor * vec4(diffuse, 1.0);
}