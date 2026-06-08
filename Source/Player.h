
#pragma once

#include "RR.h"

class Player : public RR::GameObject
{
public:
    GAMEOBJECT(Player, RR::GameObject)
    Player();
    ~Player() override;

    void Init() override;
    void PreUpdate(float _deltaTime) override;
    void Update(float _deltaTime) override;



private:
    RR::AnimationComponent* m_animationComp = nullptr;
    RR::AudioSourceComponent* m_audioComp = nullptr;
    RR::PlayerControllerComponent* m_PCC = nullptr;

    RR::ComponentAudioTracker m_shootSfx;
    RR::ComponentAudioTracker m_walkSfx;
    RR::ComponentAudioTracker m_jumpSfx;
    RR::GameObject* m_muzzle = nullptr;

    float m_shootCooldown = 0.0f;
};


