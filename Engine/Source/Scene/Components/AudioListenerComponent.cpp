
#include "AudioListenerComponent.h"

#include "Engine.h"
#include "Scene/GameObject.h"

namespace RR
{
    AudioListenerComponent::AudioListenerComponent()
    = default;

    AudioListenerComponent::~AudioListenerComponent()
    = default;

    void AudioListenerComponent::Update(float _deltaTime)
    {
        auto pos = m_owner->GetWorldPosition();
        Engine::GetInstance().GetAudioManager().SetListenerPosition(pos);
    }

    void AudioListenerComponent::OnDisable()
    {
        Engine::GetInstance().GetAudioManager().SetListenerPosition(vec3(0.f,0.f,0.f));
    }
}
