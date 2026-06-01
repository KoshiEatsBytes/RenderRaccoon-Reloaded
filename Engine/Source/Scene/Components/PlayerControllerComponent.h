
#pragma once

#include "scene/Component.h"

namespace RR
{
    class PlayerControllerComponent : public Component
    {
    public:
        COMPONENT(PlayerControllerComponent)

        PlayerControllerComponent();
        ~PlayerControllerComponent() override;

        void Init() override;
        void Update(float _deltaTime) override;

    private:
        float m_sensitivity = 2.0f;
        float m_moveSpeed = 2.0f;
    };
}

