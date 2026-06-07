
#pragma once
#include "Scene/Component.h"
#include "Helpers/Types.h"   // vec3

namespace RR
{
    class AudioListenerComponent : public Component
    {
    public:
        COMPONENT(AudioListenerComponent)

        AudioListenerComponent() = default;
        ~AudioListenerComponent() override = default;

        void Update(float _deltaTime) override;
        void LateUpdate(float _deltaTime) override;

    private:
        // for finite difference listener velocity
        vec3 m_lastPos {0.0f};
        bool m_hasLastPos = false;
    };
}
