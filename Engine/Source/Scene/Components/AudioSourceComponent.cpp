
#include "AudioSourceComponent.h"

#include "Engine.h"
#include "Helpers/Printer.hpp"
#include "Scene/GameObject.h"

namespace RR
{
    // PUBLIC ----------------------------------------------------------------------------------------------------------

    void AudioSourceComponent::Update(float) {}

    void AudioSourceComponent::LateUpdate(float _deltaTime)
    {
        const vec3 pos = m_owner->GetWorldPosition();
        const vec3 fwd = m_owner->GetWorldRotation() * vec3(0.0f, 0.0f, -1.0f);
        const vec3 vel = ComputeVelocity(pos, _deltaTime);

        for (auto& [key, voice] : m_voices)
        {
            if (!voice->IsPlaying()) continue;
            voice->SetPosition(pos);
            voice->SetDirection(fwd);   // cone
            voice->SetVelocity(vel);    // doppler
        }

        // reap finished one-shots
        std::erase_if(m_voices, [](const auto& entry)
        {
            return entry.second->IsFinished();
        });
    }

    // TRACKERS --------------------------------------------------------------------------------------------------------

    Tracker<SpatialAudio> AudioSourceComponent::BindTrack(const std::string& _key, uInt _channel)
    {
        auto voice = Engine::GetInstance().GetAudioManager().CreateSpatial(_key, _channel);
        if (!voice)
        {
            Warn("[AUDIO SOURCE] BindTrack with key '", _key, "' failed");
            return {};
        }

        ApplyProfile(voice);
        m_voices[_key] = voice;

        return Tracker<SpatialAudio>(voice, {});
    }

    Tracker<SpatialAudio> AudioSourceComponent::GetTrack(const std::string& _key)
    {
        auto it = m_voices.find(_key);
        return it != m_voices.end() ? Tracker<SpatialAudio>(it->second, {}) : Tracker<SpatialAudio>{};
    }

    // HANDLES ---------------------------------------------------------------------------------------------------------

    void AudioSourceComponent::Play(const std::string& _key, bool _loop)
    {
        auto it = m_voices.find(_key);
        if (it != m_voices.end()) it->second->Play(_loop);
    }

    void AudioSourceComponent::Stop(const std::string& _key, float _fade)
    {
        auto it = m_voices.find(_key);
        if (it == m_voices.end()) return;

        if (_fade > 0.0f)
            it->second->FadeOut(_fade);
        else
            it->second->Stop();
    }

    void AudioSourceComponent::Pause(const std::string& _key)
    {
        auto it = m_voices.find(_key);
        if (it != m_voices.end()) it->second->Pause();
    }

    void AudioSourceComponent::Resume(const std::string& _key)
    {
        auto it = m_voices.find(_key);
        if (it != m_voices.end()) it->second->Resume();
    }

    bool AudioSourceComponent::IsPlaying(const std::string& _key) const
    {
        auto it = m_voices.find(_key);
        return it != m_voices.end() && it->second->IsPlaying();
    }

    void AudioSourceComponent::StopAll(float _fade)
    {
        for (auto& [key, voice] : m_voices)
        {
            if (_fade > 0.0f)
                voice->FadeOut(_fade);
            else
                voice->Stop();
        }
    }

    // SPATIALIZATION --------------------------------------------------------------------------------------------------

    void AudioSourceComponent::SetMinMaxDistance(float _min, float _max)
    {
        m_minDistance = _min;
        m_maxDistance = _max;

        for (auto& [key, voice] : m_voices)
        {
            voice->SetMinMaxDistance(_min, _max);
        }
    }

    void AudioSourceComponent::SetRolloff(float _rolloff)
    {
        m_rolloff = _rolloff;

        for (auto& [key, voice] : m_voices)
        {
            voice->SetRolloff(_rolloff);
        }
    }

    void AudioSourceComponent::SetAttenuationModel(AttenuationModel _model)
    {
        m_attenuation = _model;

        for (auto& [key, voice] : m_voices)
        {
            voice->SetAttenuationModel(_model);
        }
    }

    void AudioSourceComponent::SetDopplerFactor(float _factor)
    {
        m_dopplerFactor = _factor;
        for (auto& [key, voice] : m_voices)
        {
            voice->SetDopplerFactor(_factor);
        }
    }

    void AudioSourceComponent::SetCone(float _innerRad, float _outerRad, float _outerGain)
    {
        m_innerAngleRad = _innerRad;
        m_outerAngleRad = _outerRad;
        m_outerGain     = _outerGain;

        for (auto& [key, voice] : m_voices)
        {
            voice->SetCone(_innerRad, _outerRad, _outerGain);
        }
    }

    // PROTECTED -------------------------------------------------------------------------------------------------------

    void AudioSourceComponent::OnEnable()
    {
        Component::OnEnable();

        // Pause all
        for (auto& [key, voice] : m_voices)
        {
            voice->Resume();
        }
    }

    void AudioSourceComponent::OnDisable()
    {
        Component::OnDisable();

        // Unpause all
        for (auto& [key, voice] : m_voices)
        {
            voice->Pause();
        }
    }

    // PRIVATE ---------------------------------------------------------------------------------------------------------

    void AudioSourceComponent::ApplyProfile(const std::shared_ptr<SpatialAudio>& _voice) const
    {
        _voice->SetMinMaxDistance(m_minDistance, m_maxDistance);
        _voice->SetRolloff(m_rolloff);
        _voice->SetAttenuationModel(m_attenuation);
        _voice->SetDopplerFactor(m_dopplerFactor);
        _voice->SetCone(m_innerAngleRad, m_outerAngleRad, m_outerGain);
    }

    vec3 AudioSourceComponent::ComputeVelocity(const vec3& _pos, float _dt)
    {
        if (!m_hasLastPos || _dt <= 0.0f)
        {
            m_lastPos = _pos;
            m_hasLastPos = true;
            return vec3(0.0f);
        }

        vec3 raw = (_pos - m_lastPos) / _dt;
        m_lastPos = _pos;

        // ignore implausible jumps
        constexpr float kMaxSpeed = 1000.0f;
        if ((raw.x * raw.x + raw.y * raw.y + raw.z * raw.z) > (kMaxSpeed * kMaxSpeed))
        {
            raw = vec3(0.0f);
        }

        m_velocity = m_velocity + (raw - m_velocity) * 0.5f;
        return m_velocity;
    }
}
