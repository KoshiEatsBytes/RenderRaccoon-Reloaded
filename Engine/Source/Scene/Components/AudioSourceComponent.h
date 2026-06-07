
#pragma once
#include <memory>
#include <string>
#include <unordered_map>

#include "Scene/Component.h"
#include "Audio/Tracker.h"

namespace RR
{
    class AudioSourceComponent : public Component
    {
    public:
        COMPONENT(AudioSourceComponent)

        AudioSourceComponent() = default;
        ~AudioSourceComponent() override = default;

        void Update(float _deltaTime) override;
        void LateUpdate(float _deltaTime) override;

        // Trackers
        Tracker<SpatialAudio> BindTrack(const std::string& _key, uInt _channel);
        Tracker<SpatialAudio> GetTrack (const std::string& _key);

        // Handles
        void Play  (const std::string& _key, bool _loop = false);
        void Stop  (const std::string& _key, float _fade = 0.0f);
        void Pause (const std::string& _key);
        void Resume(const std::string& _key);
        bool IsPlaying(const std::string& _key) const;
        void StopAll(float _fade = 0.0f);

        // Spatialization
        void SetMinMaxDistance(float _min, float _max);
        void SetRolloff(float _rolloff);
        void SetAttenuationModel(AttenuationModel _model);
        void SetDopplerFactor(float _factor);
        void SetCone(float _innerRad, float _outerRad, float _outerGain);

    protected:
        void OnEnable() override;
        void OnDisable() override;

    private:
        void ApplyProfile(const std::shared_ptr<SpatialAudio>& _voice) const;
        vec3 ComputeVelocity(const vec3& _pos, float _dt);

        std::unordered_map<std::string, std::shared_ptr<SpatialAudio>> m_voices;
        std::vector<std::shared_ptr<SpatialAudio>> m_oneShots;

        // velocity
        vec3 m_lastPos {0.0f};
        vec3 m_velocity {0.0f};
        bool m_hasLastPos = false;

        // Spatialization defaults
        float m_minDistance   = 1.0f;
        float m_maxDistance   = 50.0f;
        float m_rolloff       = 1.0f;
        float m_dopplerFactor = 1.0f;
        AttenuationModel m_attenuation = AttenuationModel::INVERSE;

        // Directional cone
        float m_innerAngleRad = 6.28318530718f;
        float m_outerAngleRad = 6.28318530718f;
        float m_outerGain     = 1.0f;
    };
}
