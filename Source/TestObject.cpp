
#include "TestObject.h"

#include "GLFW/glfw3.h"

TestObject::TestObject()
{
    const std::string vertPath = std::string(ASSETS_PATH) + "Shaders/basic.vert";
    const std::string fragPath = std::string(ASSETS_PATH) + "Shaders/basic.frag";

    auto& graphicsAPI = RR::Engine::GetInstance().GetGraphicsAPI();

    auto shaderProgram = graphicsAPI.CreateShaderProgram(vertPath, fragPath);

    m_mat.SetShaderProgram(shaderProgram);

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

    m_mesh = std::make_shared<RR::Mesh>(vertexLayout, vertices, indeces);
}

void TestObject::Update(float _deltaTime)
{
    GameObject::Update(_deltaTime);

    auto& input = RR::Engine::GetInstance().GetInputManager();

    if (input.IsKeyPressed(GLFW_KEY_A))
    {
        m_offsetX -= 0.001f;
    }
    if (input.IsKeyPressed(GLFW_KEY_D))
    {
        m_offsetX += 0.001f;
    }
    if (input.IsKeyPressed(GLFW_KEY_W))
    {
        m_offsetY += 0.001f;
    }
    if (input.IsKeyPressed(GLFW_KEY_S))
    {
        m_offsetY -= 0.001f;
    }

    m_mat.SetParam("uOffset", m_offsetX, m_offsetY);

    RR::RenderCommand command;
    command.material = &m_mat;
    command.mesh = m_mesh.get();

    auto& renderQueue = RR::Engine::GetInstance().GetRenderQueue();
    renderQueue.Submit(command);
}
