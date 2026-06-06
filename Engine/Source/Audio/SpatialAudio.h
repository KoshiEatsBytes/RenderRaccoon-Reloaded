
#pragma once
#include "Voice/AudioVoice.h"

namespace RR
{
    // Engine side enum that maps to miniaudio
    enum class AttenuationModel
    {
        NONE,
        INVERSE,
        LINEAR,
        EXPONENTIAL
    };

    class SpatialAudio : public AudioVoice
    {
    public:
        SpatialAudio(std::shared_ptr<AudioClip> _clip, maEngine* _engine);

        // Per frame, ideally driven by AudioSourceComponent
        void SetDirection(const vec3& _dir);
        void SetVelocity (const vec3& _vel);
        void SetPosition (const vec3& _pos);
        vec3 GetPosition () const;

        // Configuration
        void SetAttenuationModel(AttenuationModel _model);
        void SetMinMaxDistance(float _min, float _max);
        void SetRolloff(float _rollOff);
        void SetDopplerFactor(float _factor);

        // Advanced
        void SetCone(float _innerRad, float _outerRad, float _outerGain);
        void SetRelativeToListener(bool _relative);

        // Gets
        AttenuationModel GetAttenuationModel() const;
        vec2 GetMinMaxDistance() const;
        float GetRollOff() const;
        float GetDopplerFactor() const;
        vec3 GetCone() const;
        bool GetRelativeToListener() const;

    private:
        AttenuationModel m_attModel = AttenuationModel::INVERSE;
    };
}
