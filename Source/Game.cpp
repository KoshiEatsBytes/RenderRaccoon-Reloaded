
#include "Game.h"
#include "GLFW/glfw3.h"

// PUBLIC --------------------------------------------------------------------------------------------------------------

Game::Game() = default;

Game::~Game() = default;

bool Game::Init()
{
    const std::string vertPath = "Shaders/basic.vert";
    const std::string fragPath = "Shaders/basic.frag";

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

    m_mesh = std::make_unique<RR::Mesh>(vertexLayout, vertices, indeces);


    return true;
}

void Game::Update(float _deltaTime)
{
    auto& input = RR::Engine::GetInstance().GetInputManager();

    if (input.IsKeyPressed(GLFW_KEY_ESCAPE))
    {
        SetShouldClose(true);
    }

    RR::RenderCommand command;
    command.material = &m_mat;
    command.mesh = m_mesh.get();

    auto& renderQueue = RR::Engine::GetInstance().GetRenderQueue();
    renderQueue.Submit(command);
}

void Game::Destroy()
{
}
