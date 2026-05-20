#pragma once

#include <pch.h>

class Game : public RR::Application
{
public:
    Game();
    ~Game();

    bool Init() override;

    void Update(const float &deltaTime) override;

    void Destroy() override;
};

