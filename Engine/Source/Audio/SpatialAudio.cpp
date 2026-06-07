
#include "SpatialAudio.h"

#include "miniaudio.h"
#include "Helpers/Printer.hpp"
#include "Voice/AudioClip.h"

namespace RR
{
    // LOCAL -----------------------------------------------------------------------------------------------------------

    static ma_attenuation_model ToMa(AttenuationModel _model)
    {
        switch (_model)
        {
            case AttenuationModel::NONE:
                return ma_attenuation_model_none;

            case AttenuationModel::LINEAR:
                return ma_attenuation_model_linear;

            case AttenuationModel::EXPONENTIAL:
                return ma_attenuation_model_exponential;

            case AttenuationModel::INVERSE:
                return ma_attenuation_model_inverse;

            default:
                return ma_attenuation_model_inverse;
        }
    }

    // PUBLIC ----------------------------------------------------------------------------------------------------------

    SpatialAudio::SpatialAudio(std::shared_ptr<AudioClip> _clip, maEngine* _engine)
        : AudioVoice(std::move(_clip), _engine)
    {
        if (!m_initialized) return;

        // Gate non-mono sources from spatial audio
        if (m_clip->IsMono())
        {
            AudioVoice::SetSpatial(true);
        }
        else
        {
            Warn("[AUDIO - SPATIAL] Non-mono clip; disabling spatialization (author spatial SFX as mono)");
            AudioVoice::SetSpatial(false);
        }

        SetAttenuationModel(AttenuationModel::INVERSE);
    }

    // COMPONENT DRIVEN ------------------------------------------------------------------------------------------------

    void SpatialAudio::SetDirection(const vec3& _dir)
    {
        if (!m_initialized)
        {
            Warn("[SPATIAL AUDIO - DIR] Tried setting direction to an uninitialized audio file");
            return;
        }

        ma_sound_set_direction(m_sound.get(), _dir.x, _dir.y, _dir.z);
    }

    void SpatialAudio::SetVelocity(const vec3& _vel)
    {
        if (!m_initialized)
        {
            Warn("[SPATIAL AUDIO - VEL] Tried setting velocity to an uninitialized audio file");
            return;
        }

        ma_sound_set_velocity(m_sound.get(), _vel.x, _vel.y, _vel.z);
    }

    void SpatialAudio::SetPosition(const vec3& _pos)
    {
        if (!m_initialized)
        {
            Warn("[SPATIAL AUDIO - POS] Tried setting position to an uninitialized audio file");
            return;
        }

        ma_sound_set_position(m_sound.get(), _pos.x, _pos.y, _pos.z);
    }

    vec3 SpatialAudio::GetPosition() const
    {
        if (!m_initialized) return vec3 {0.0f};

        const auto [x, y, z] = ma_sound_get_position(m_sound.get());
        return vec3 {x, y, z};
    }

    // ADVANCED --------------------------------------------------------------------------------------------------------

    void SpatialAudio::SetAttenuationModel(AttenuationModel _model)
    {
        if (!m_initialized)
        {
            Warn("[SPATIAL AUDIO - ATTENUATION] Tried setting attenuation model to an uninitialized audio file");
            return;
        }

        ma_sound_set_attenuation_model(m_sound.get(), ToMa(_model));
        m_attModel = _model;
    }

    void SpatialAudio::SetMinMaxDistance(float _min, float _max)
    {
        if (!m_initialized)
        {
            Warn("[SPATIAL AUDIO - MIN MAX] Tried setting MinMax distance to an uninitialized audio file");
            return;
        }

        ma_sound_set_min_distance(m_sound.get(), _min);
        ma_sound_set_max_distance(m_sound.get(), _max);
    }

    void SpatialAudio::SetRolloff(float _rollOff)
    {
        if (!m_initialized)
        {
            Warn("[SPATIAL AUDIO - ROLLOFF] Tried setting rolloff to an uninitialized audio file");
            return;
        }

        ma_sound_set_rolloff(m_sound.get(), _rollOff);
    }

    void SpatialAudio::SetDopplerFactor(float _factor)
    {
        if (!m_initialized)
        {
            Warn("[SPATIAL AUDIO - DOPPLER] Tried setting doppler to an uninitialized audio file");
            return;
        }

        ma_sound_set_doppler_factor(m_sound.get(), _factor);
    }

    void SpatialAudio::SetCone(float _innerRad, float _outerRad, float _outerGain)
    {
        if (!m_initialized)
        {
            Warn("[SPATIAL AUDIO - CONE] Tried setting cone to an uninitialized audio file");
            return;
        }

        ma_sound_set_cone(m_sound.get(), _innerRad, _outerRad, _outerGain);
    }

    void SpatialAudio::SetRelativeToListener(bool _relative)
    {
        if (!m_initialized)
        {
            Warn("[SPATIAL AUDIO - RELATIVE] Tried setting audio relative to listener to an uninitialized audio file");
            return;
        }

        ma_sound_set_positioning(m_sound.get(), _relative ? ma_positioning_relative : ma_positioning_absolute);
    }

    void SpatialAudio::CloneSettings(const SpatialAudio& _from)
    {
        if (!m_initialized) return;

        SetVolume(_from.GetVolume());
        SetPitch (_from.GetPitch());

        // Spatial profile
        SetAttenuationModel(_from.GetAttenuationModel());

        const vec2 mm = _from.GetMinMaxDistance();
        SetMinMaxDistance(mm.x, mm.y);

        SetRolloff(_from.GetRollOff());
        SetDopplerFactor(_from.GetDopplerFactor());

        const vec3 cone = _from.GetCone();
        SetCone(cone.x, cone.y, cone.z);

        SetRelativeToListener(_from.GetRelativeToListener());
    }

    void SpatialAudio::SetSpatialization(bool _spatial)
    {
        AudioVoice::SetSpatial(_spatial);
    }

    AttenuationModel SpatialAudio::GetAttenuationModel() const
    {
        return m_attModel;
    }

    vec2 SpatialAudio::GetMinMaxDistance() const
    {
        if (!m_initialized) return vec2 {0.0f};

        const float max = ma_sound_get_max_distance(m_sound.get());
        const float min = ma_sound_get_min_distance(m_sound.get());
        return vec2 {min, max};
    }

    float SpatialAudio::GetRollOff() const
    {
        if (!m_initialized) return 0.0f;

        return ma_sound_get_rolloff(m_sound.get());
    }

    float SpatialAudio::GetDopplerFactor() const
    {
        if (!m_initialized) return 0.0f;

        return ma_sound_get_doppler_factor(m_sound.get());
    }

    vec3 SpatialAudio::GetCone() const
    {
        if (!m_initialized) return vec3 {0.0f};

        float innRad, outRad, outGain;
        ma_sound_get_cone(m_sound.get(), &innRad, &outRad, &outGain);
        return vec3 {innRad, outRad, outGain};
    }

    bool SpatialAudio::GetRelativeToListener() const
    {
        if (!m_initialized) return false;

        switch (ma_sound_get_positioning(m_sound.get()))
        {
            case ma_positioning_absolute:
                return false;

            case ma_positioning_relative:
                return true;
        }

        return false;
    }
}
