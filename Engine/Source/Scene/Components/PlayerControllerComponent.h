
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

        void Update(float _deltaTime) override;

    private:
        float m_sensitivity = 1.0f;
        float m_moveSpeed = 1.0f;
    };
}

