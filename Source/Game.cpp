
#include "Game.h"

#include "TestObject.h"
#include "GLFW/glfw3.h"

// PUBLIC --------------------------------------------------------------------------------------------------------------

Game::Game() = default;

Game::~Game() = default;

bool Game::Init()
{
    m_scene.CreateObject<TestObject>("TestObject");

    return true;
}

void Game::Update(float _deltaTime)
{
    m_scene.Update(_deltaTime);
}

void Game::Destroy()
{
}
