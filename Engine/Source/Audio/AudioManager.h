
#pragma once
#include <memory>

#include "Helpers/Common.h"

namespace RR
{
    class AudioManager
    {
        AudioManager();
        ~AudioManager();

    public:
        friend class Engine;

        // Delete copy and move
        AudioManager(const AudioManager&) = delete;
        AudioManager& operator=(const AudioManager&) = delete;
        AudioManager(AudioManager&&) noexcept = delete;
        AudioManager& operator=(AudioManager&&) noexcept = delete;

        bool Init();

        maEngine* GetAudioEngine() const;

        void SetListenerParams(const vec3& _pos, const vec3& _dir, const vec3& _up) const;
        void SetListenerDirection(const vec3& _dir) const;
        void SetListenerWorldUp(const vec3& _up) const;
        void SetListenerPosition(const vec3& _pos) const;

    private:
        bool m_initialized = false;
        std::unique_ptr<maEngine> m_audioEngine;
    };
}
