
#include <miniaudio.h>

#include "AudioManager.h"

#include "Helpers/Printer.hpp"

namespace RR
{
    // PUBLIC ----------------------------------------------------------------------------------------------------------

    AudioManager::AudioManager()
    {
        m_audioEngine = std::make_unique<maEngine>();
    }

    AudioManager::~AudioManager()
    {
        if (m_initialized)
        {
            ma_engine_uninit(GetAudioEngine());
        }
    }

    bool AudioManager::Init()
    {
        auto result = ma_engine_init(nullptr, GetAudioEngine());

        if (result != MA_SUCCESS)
        {
            ma_engine_uninit(GetAudioEngine());
            Error("[AUDIO - INIT] MiniAudio Engine has failed to initialize!");
            return false;
        }

        m_initialized = true;
        return true;
    }

    maEngine* AudioManager::GetAudioEngine() const
    {
        return m_audioEngine.get();
    }

    void AudioManager::SetListenerParams(const vec3& _pos, const vec3& _dir, const vec3& _up) const
    {
        if (m_audioEngine)
        {
            ma_engine_listener_set_position (GetAudioEngine(), 0, _pos.x, _pos.y, _pos.z);
            ma_engine_listener_set_direction(GetAudioEngine(), 0, _dir.x, _dir.y, _dir.z);
            ma_engine_listener_set_world_up (GetAudioEngine(), 0, _up.x,  _up.y,  _up.z);
            return;
        }

        Warn("[AUDIO MANAGER - LISTENER PARAMS] Tried setting listener parameters with invalid audio engine");
    }

    void AudioManager::SetListenerDirection(const vec3 &_dir) const
    {
        if (m_audioEngine)
        {
            ma_engine_listener_set_direction(GetAudioEngine(), 0, _dir.x, _dir.y, _dir.z);
            return;
        }

        Warn("[AUDIO MANAGER - LISTENER DIR] Tried setting listener direction with invalid audio engine");
    }

    void AudioManager::SetListenerWorldUp(const vec3& _up) const
    {
        if (m_audioEngine)
        {
            ma_engine_listener_set_world_up(GetAudioEngine(), 0, _up.x, _up.y, _up.z);
            return;
        }

        Warn("[AUDIO MANAGER - LISTENER UP] Tried setting listener world up with invalid audio engine");
    }

    void AudioManager::SetListenerPosition(const vec3& _pos) const
    {
        if (m_audioEngine)
        {
            ma_engine_listener_set_position(GetAudioEngine(), 0, _pos.x, _pos.y, _pos.z);
            return;
        }

        Warn("[AUDIO MANAGER - LISTENER POS] Tried setting listener position with invalid audio engine");
    }
}
