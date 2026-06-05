
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

        void SetListenerPosition(const vec3& _pos) const;

    private:
        maEngine* m_audioEngine = nullptr;
    };
}
