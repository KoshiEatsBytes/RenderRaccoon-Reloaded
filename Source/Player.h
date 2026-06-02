
#pragma once

#include "RR.h"

class Player : public RR::GameObject
{
public:
    Player();
    ~Player() override;

    void Init() override;
    void Update(float _deltaTime) override;

private:
    RR::AnimationComponent* m_animationComp = nullptr;
};


