
#include "TestObject.h"

#include "GLFW/glfw3.h"

TestObject::TestObject()
{
    const std::string vertPath = "Shaders/basic.vert";
    const std::string fragPath = "Shaders/basic.frag";

    auto& graphicsAPI = RR::Engine::GetInstance().GetGraphicsAPI();

    auto shaderProgram = graphicsAPI.CreateShaderProgram(vertPath, fragPath);

    auto material = std::make_shared<RR::Material>();
    material->SetShaderProgram(shaderProgram);

    // Triangle made with vertices
    // Color of each vertices is on the right
    std::vector<float> vertices
    {
        0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
        -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f,
        0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 0.0f
    };

    // Instead of re-rendering vertices, use them from the existent array
    std::vector<unsigned int> indeces =
    {
        0, 1, 2,
        0, 2, 3
    };

    RR::VertexLayout vertexLayout;

    // Position
    vertexLayout.elements.push_back({
        0,
        3,
        GL_FLOAT,
        0
    });

    // Color
    vertexLayout.elements.push_back({
        1,
        3,
        GL_FLOAT,
        sizeof(float) * 3
        });

    // Stride
    vertexLayout.stride = sizeof(float) * 6;

    auto mesh = std::make_shared<RR::Mesh>(vertexLayout, vertices, indeces);

    AddComponent(new RR::MeshComponent(material, mesh));
}

void TestObject::Update(float _deltaTime)
{
    GameObject::Update(_deltaTime);

    // auto position = GetPosition();
    // auto& input = RR::Engine::GetInstance().GetInputManager();
    //
    // if (input.IsKeyPressed(GLFW_KEY_A))
    // {
    //     position.x -= 0.001f;
    // }
    // if (input.IsKeyPressed(GLFW_KEY_D))
    // {
    //     position.x += 0.001f;
    // }
    // if (input.IsKeyPressed(GLFW_KEY_W))
    // {
    //     position.y += 0.001f;
    // }
    // if (input.IsKeyPressed(GLFW_KEY_S))
    // {
    //     position.y -= 0.001f;
    // }
    //
    // SetPosition(position);
}
