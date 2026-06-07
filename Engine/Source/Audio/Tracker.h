
#pragma once
#include <memory>
#include <functional>
#include <concepts>

#include "Helpers/Types.h" // vec3
#include "StaticAudio.h"
#include "SpatialAudio.h"

namespace RR
{
    /**
     * @brief A non-owning handle to a voice that lives inside a channel.
     */
    template<typename T>
    class Tracker
    {
        static_assert(
            std::same_as<T, StaticAudio> ||
            std::same_as<T, SpatialAudio>,
            "Tracker<T> supports only StaticAudio or SpatialAudio");

        // Proxy to keep the voice alive
        struct Access
        {
            std::shared_ptr<T> voice;
            T* operator->() const
            {
                return voice.get();
            }
        };

    public:
        using Reviver = std::function<std::shared_ptr<T>()>;

        Tracker() = default;
        Tracker(std::weak_ptr<T> _voice, Reviver _revive)
            : m_voice(std::move(_voice)), m_revive(std::move(_revive)) {

        }

        // Lifecycle, play revives a dead voice
        void Play(bool _loop = false)
        {
            auto voice = m_voice.lock();

            if (!voice)
            {
                if (!m_revive) return;
                voice = m_revive();

                if (!voice) return;
                m_voice = voice;
                ApplyCache(voice);
            }

            voice->Play(_loop);
        }

        void Stop(float _fadeOut = 0.0f)
        {
            if (auto voice = m_voice.lock())
            {
                if (_fadeOut > 0.0f)
                    voice->FadeOut(_fadeOut);
                else
                    voice->Stop();
            }
        }

        void Pause()
        {
            if (auto voice = m_voice.lock())
            {
                voice->Pause();
            }
        }

        void Resume()
        {
            if (auto voice = m_voice.lock())
            {
                voice->Resume();
            }
        }

        // Sticky configs, cached, re-applied automatically on revive
        void SetVolume(float _vol)
        {
            m_volume = _vol;

            if (auto voice = m_voice.lock())
            {
                voice->SetVolume(_vol);
            }
        }

        void SetPitch (float _pitch)
        {
            m_pitch  = _pitch;
            if (auto voice = m_voice.lock())
            {
                voice->SetPitch(_pitch);
            }
        }

        void SetPan(float _pan) requires std::same_as<T, StaticAudio>
        {
            m_pan = _pan;
            if (auto voice = m_voice.lock())
            {
                voice->SetPan(_pan);
            }
        }

        // Per-frame spatial
        void SetPosition(const vec3& _pos) requires std::same_as<T, SpatialAudio>
        {
            if (auto voice = m_voice.lock())
            {
                voice->SetPosition(_pos);
            }
        }

        void SetDirection(const vec3& _dir) requires std::same_as<T, SpatialAudio>
        {
            if (auto voice = m_voice.lock())
            {
                voice->SetDirection(_dir);
            }
        }

        void SetVelocity(const vec3& _vel) requires std::same_as<T, SpatialAudio>
        {
            if (auto voice = m_voice.lock())
            {
                voice->SetVelocity(_vel);
            }
        }

        // State
        bool IsValid() const
        {
            return !m_voice.expired();
        }

        bool IsPlaying() const
        {
            auto voice = m_voice.lock();
            return voice && voice->IsPlaying();
        }

        bool IsFinished() const
        {
            auto voice = m_voice.lock();
            return voice && voice->IsFinished();
        }

        explicit operator bool() const
        {
            return IsValid();
        }

        // Direct access to the raw voice via ->
        Access operator->() const
        {
            return Access{ m_voice.lock() };
        }

        std::shared_ptr<T> Lock() const
        {
            return m_voice.lock();
        }

    private:
        void ApplyCache(const std::shared_ptr<T>& _voice) const
        {
            _voice->SetVolume(m_volume);
            _voice->SetPitch(m_pitch);

            if constexpr (std::same_as<T, StaticAudio>)
            {
                _voice->SetPan(m_pan);
            }
        }

        std::weak_ptr<T> m_voice;
        Reviver m_revive;

        // sticky cache
        float m_volume = 1.0f;
        float m_pitch  = 1.0f;
        float m_pan    = 0.0f;
    };
}
