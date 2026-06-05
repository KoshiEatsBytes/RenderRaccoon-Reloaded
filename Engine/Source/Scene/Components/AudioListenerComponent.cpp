
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
    }

    void AudioListenerComponent::LateUpdate(float _deltaTime)
    {
        const quat rot       = m_owner->GetWorldRotation();
        const vec3 pos       = m_owner->GetWorldPosition();
        const vec3 direction = rot * vec3(0.0f, 0.0f, -1.0f);
        const vec3 up        = rot * vec3(0.0f, 1.0f,  0.0f);

        Engine::GetInstance().GetAudioManager().SetListenerParams(pos, direction, up);
    }

    void AudioListenerComponent::OnDisable()
    {
        Engine::GetInstance().GetAudioManager().SetListenerPosition(vec3(0.f,0.f,0.f));
    }
}
