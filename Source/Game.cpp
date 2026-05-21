
#include "Game.h"
#include "GLFW/glfw3.h"

// PUBLIC --------------------------------------------------------------------------------------------------------------

Game::Game() = default;

Game::~Game() = default;

bool Game::Init()
{
    return true;
}

void Game::Update(const float &deltaTime)
{
    // auto& input = RR::Engine::GetInstance().GetInputManager();
    //
    // if (input.IsKeyPressed(GLFW_KEY_LEFT_SHIFT))
    // {
    //     RR::print("SHIFT button is pressed");
    // }
}

void Game::Destroy()
{
}
