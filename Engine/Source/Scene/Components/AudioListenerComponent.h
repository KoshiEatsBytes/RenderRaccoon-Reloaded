
#pragma once
#include "Scene/Component.h"

namespace RR
{
    class AudioListenerComponent : public Component
    {
    public:
        COMPONENT(AudioListenerComponent)

        AudioListenerComponent();
        ~AudioListenerComponent() override;

        void Update(float _deltaTime) override;

    protected:
        void OnDisable() override;
    };
}

