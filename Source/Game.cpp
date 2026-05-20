
#include "Game.h"
#include "GLFW/glfw3.h"

Game::Game() = default;

Game::~Game() = default;

bool Game::Init()
{
    return true;
}

void Game::Update(const float &deltaTime)
{
    //rr::print("Current Delta: ", deltaTime);

    auto& input = RR::Engine::GetInstance().GetInputManager();

    if (input.IsKeyPressed(GLFW_KEY_A))
    {
        RR::print("A button is pressed");
    }
}

void Game::Destroy()
{
}
