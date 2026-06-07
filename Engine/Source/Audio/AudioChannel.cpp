
#include <miniaudio.h>
#include <algorithm>

#include "AudioChannel.h"
#include "Voice/AudioVoice.h"
#include "Helpers/Printer.hpp"

namespace RR
{
    // PUBLIC ----------------------------------------------------------------------------------------------------------

    AudioChannel::AudioChannel()
    = default;

    AudioChannel::~AudioChannel()
    {
        // Destroy the voices first
        m_tracks.clear();
        m_oneShots.clear();

        if (m_initialized)
        {
            ma_sound_group_uninit(m_group.get());
        }
    }

    bool AudioChannel::Init(maEngine* _engine)
    {
        if (!_engine)
        {
            Warn("[AUDIO - CHANNEL] Tried initializing a channel with an INVALID engine");
            return false;
        }

        m_group = std::make_unique<maSoundGroup>();
        ma_result result = ma_sound_group_init(
            _engine,
            0,
            nullptr,
            m_group.get()
            );

        if (result != MA_SUCCESS)
        {
            Warn("[AUDIO - CHANNEL] Failed to initialize the channel's sound group");
            m_group.reset();
            return false;
        }

        // Push any values set before Init onto the group
        ma_sound_group_set_volume(m_group.get(), m_volume);
        ma_sound_group_set_pitch (m_group.get(), m_pitch);
        ma_sound_group_set_pan   (m_group.get(), m_pan);

        m_initialized = true;
        return true;
    }

    void AudioChannel::Update()
    {
        // One-shots are clones of a cached voice: reap once playback reaches the end.
        std::erase_if(m_oneShots, [](const std::shared_ptr<AudioVoice>& voice)
        {
            return voice->IsFinished();
        });
    }

    void AudioChannel::Add(const std::string& _name, std::shared_ptr<AudioVoice> _voice)
    {
        if (!_voice)
        {
            Warn("[AUDIO - CHANNEL] Tried adding an INVALID voice to a channel");
            return;
        }

        if (m_initialized) _voice->AttachToGroup(m_group.get());
        m_tracks[_name] = std::move(_voice);
    }

    void AudioChannel::AddOneShot(std::shared_ptr<AudioVoice> _voice)
    {
        if (!_voice)
        {
            Warn("[AUDIO - CHANNEL] Tried adding an INVALID one-shot to a channel");
            return;
        }

        if (m_initialized) _voice->AttachToGroup(m_group.get());
        m_oneShots.push_back(std::move(_voice));
    }

    void AudioChannel::Stop(const std::string& _name, float _fadeOut)
    {
        auto it = m_tracks.find(_name);
        if (it == m_tracks.end()) return;

        if (_fadeOut > 0.0f)
            it->second->FadeOut(_fadeOut);
        else
            it->second->Stop();
    }

    void AudioChannel::StopAll(float _fadeOut)
    {
        for (auto& [name, voice] : m_tracks)
        {
            if (_fadeOut > 0.0f)
                voice->FadeOut(_fadeOut);
            else
                voice->Stop();
        }
    }

    std::shared_ptr<AudioVoice> AudioChannel::Find(const std::string& _name) const
    {
        auto it = m_tracks.find(_name);
        if (it != m_tracks.end())
        {
            return it->second;
        }
        return nullptr;
    }

    bool AudioChannel::Contains(const std::string& _name) const
    {
        return m_tracks.contains(_name);
    }

    // BUS CONTROLS ----------------------------------------------------------------------------------------------------

    void AudioChannel::SetVolume(float _vol)
    {
        m_volume = std::clamp(_vol, 0.0f, 1.0f);
        if (m_initialized) ma_sound_group_set_volume(m_group.get(), m_volume);
    }

    float AudioChannel::GetVolume() const { return m_volume; }

    void AudioChannel::SetPitch(float _pitch)
    {
        m_pitch = std::max(_pitch, 0.01f);
        if (m_initialized) ma_sound_group_set_pitch(m_group.get(), m_pitch);
    }

    float AudioChannel::GetPitch() const { return m_pitch; }

    void AudioChannel::SetPan(float _pan)
    {
        m_pan = std::clamp(_pan, -1.0f, 1.0f);
        if (m_initialized) ma_sound_group_set_pan(m_group.get(), m_pan);
    }

    float AudioChannel::GetPan() const
    {
        return m_pan;
    }

    maSoundGroup* AudioChannel::GetGroup() const
    {
        return m_group.get();
    }
}
