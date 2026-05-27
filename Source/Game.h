#pragma once

#include <RR.h>
#include <memory>

class Game : public RR::Application
{
public:
    Game();
    ~Game() override;

    bool Init()                         override;
    void Update(float _deltaTime)       override;
    void Destroy()                      override;

private:
    RR::Scene* m_scene = nullptr;
};


