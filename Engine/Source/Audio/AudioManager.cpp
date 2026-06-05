
#include <miniaudio.h>

#include "AudioManager.h"

#include "Helpers/Printer.hpp"

namespace RR
{
    // PUBLIC ----------------------------------------------------------------------------------------------------------

    AudioManager::AudioManager()
    {
        m_audioEngine = new maEngine;
    }

    AudioManager::~AudioManager()
    {
        if (m_audioEngine)
        {
            ma_engine_uninit(m_audioEngine);
            delete m_audioEngine;
        }
    }

    bool AudioManager::Init()
    {
        auto result = ma_engine_init(nullptr, m_audioEngine);

        if (result != MA_SUCCESS)
        {
            ma_engine_uninit(m_audioEngine);
            delete m_audioEngine;

            Error("[AUDIO - INIT] MiniAudio Engine has failed to initialize!");
            return false;
        }

        return true;
    }

    maEngine* AudioManager::GetAudioEngine() const
    {
        return m_audioEngine;
    }

    void AudioManager::SetListenerPosition(const vec3& _pos) const
    {
        if (m_audioEngine)
        {
            ma_engine_listener_set_position(m_audioEngine, 0, _pos.x, _pos.y, _pos.z);
        }
    }
}
