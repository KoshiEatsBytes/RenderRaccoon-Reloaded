
#include "Game.h"

Game::Game() = default;

Game::~Game() = default;

bool Game::Init()
{
    return true;
}

void Game::Update(const float &deltaTime)
{
    //rr::print("Current Delta: ", deltaTime);
}

void Game::Destroy()
{
}
