#pragma once

#include <RR.h>

#include "Player.h"

class Game : public RR::Scene
{
public:
    Game();
    ~Game() override;

protected:
    bool Init()                         override;
    void PreUpdate(float _deltaTime)    override;
    void Update(float _deltaTime)       override;
    void LateUpdate(float _deltaTime)   override;
    void Destroy()                      override;
};


