
#include "AudioListenerComponent.h"

#include "Engine.h"
#include "Scene/GameObject.h"

namespace RR
{
    void AudioListenerComponent::Update(float) {}

    void AudioListenerComponent::LateUpdate(float _deltaTime)
    {
        const quat rot = m_owner->GetWorldRotation();
        const vec3 pos = m_owner->GetWorldPosition();
        const vec3 dir = rot * vec3(0.0f, 0.0f, -1.0f);
        const vec3 up  = rot * vec3(0.0f, 1.0f,  0.0f);

        vec3 vel(0.0f);
        if (m_hasLastPos && _deltaTime > 0.0f) vel = (pos - m_lastPos) / _deltaTime;
        m_lastPos = pos;
        m_hasLastPos = true;

        auto& audio = Engine::GetInstance().GetAudioManager();
        audio.SetListenerParams(pos, dir, up);
        audio.SetListenerVelocity(vel);
    }
}
