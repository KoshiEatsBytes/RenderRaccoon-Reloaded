
#include "Game.h"
#include "GLFW/glfw3.h"

// PUBLIC --------------------------------------------------------------------------------------------------------------

Game::Game() = default;

Game::~Game() = default;

bool Game::Init()
{
    // "Shaders/basic.vert"
    // "Shaders/basic.frag"

    auto& graphicsAPI = RR::Engine::GetInstance().GetGraphicsAPI();

    auto shaderProgram = graphicsAPI.CreateShaderProgram("Shaders/basic.vert", "Shaders/basic.frag");
    

    return true;
}

void Game::Update(const float &deltaTime)
{
    auto& input = RR::Engine::GetInstance().GetInputManager();

    if (input.IsKeyPressed(GLFW_KEY_ESCAPE))
    {
        SetShouldClose(true);
    }
}

void Game::Destroy()
{
}
