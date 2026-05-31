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
uniform vec3 uCameraPos;

void main()
{
    vec3 norm = normalize(vNormal);

    // Diffuse Lightitng
    vec3 lightDir = normalize(uLight.position - vFragPos);
    float diffFac = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diffFac * uLight.color;

    // Specular Lighting
    vec3 viewDir = normalize(uCameraPos - vFragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    // dot measures alignment of cam and reflection
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0); //TODO: 32 should be var
    float specularStrenght = 0.5; //TODO: this should be a var
    vec3 specular = specularStrenght * spec * uLight.color;

    vec3 result = diffuse + specular;

    vec4 texColor = texture(baseColorTexture, vUV);

    FragColor = texColor * vec4(result, 1.0);
}