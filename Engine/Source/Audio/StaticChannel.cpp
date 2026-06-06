
#include "StaticChannel.h"
#include "StaticAudio.h"
#include "Helpers/Printer.hpp"

namespace RR
{
    // PUBLIC ----------------------------------------------------------------------------------------------------------

    StaticChannel::StaticChannel()
    = default;

    StaticChannel::~StaticChannel()
    = default;

    void StaticChannel::Update()
    {
        // kill oneshots when they finish
        std::erase_if(m_oneShots,
            [](const std::unique_ptr<StaticAudio>& voice) {
                return voice->IsFinished();
            });

        // Erase when its been told to stop and has actually stopped
        std::erase_if(m_tracks, [](const auto& killVoice)
            {
                const Track& track = killVoice.second;

                if (track.voice->IsFinished() || (track.removing && !track.voice->IsPlaying()))
                {
                    return true;
                }
                return false;
            }
        );
    }

    void StaticChannel::AddOneShot(const std::string& _name, std::unique_ptr<StaticAudio> _voice, float _vol)
    {
        if (!_voice)
        {
            Warn("[AUDIO - STATIC CHANNEL] Tried adding INVALID voice to audio channel");
            return;
        }

        float volume = std::clamp(_vol * m_channelVol, 0.0f, 1.0f);
        _voice->SetVolume(volume);
        _voice->SetPitch(m_channelPitch);
        _voice->SetPanMode(m_channelPanMode);
        _voice->Play(false);

        m_oneShots.push_back(std::move(_voice));
    }

    void StaticChannel::Add(const std::string& _name, std::unique_ptr<StaticAudio> _voice, bool _loop, float _fadeIn)
    {
        if (!_voice)
        {
            Warn("[AUDIO - STATIC CHANNEL] Tried adding INVALID voice to audio channel");
            return;
        }

        _voice->SetVolume(m_channelVol);
        _voice->SetPitch(m_channelPitch);
        _voice->SetPanMode(m_channelPanMode);

        if (_fadeIn > 0.0f)
            _voice->FadeIn(_fadeIn, _loop);
        else
            _voice->Play(_loop);

        m_tracks[_name] = Track {
            std::move(_voice),
            false
        };
    }

    void StaticChannel::Stop(const std::string& _name, float _fadeOut)
    {
        auto it = m_tracks.find(_name);
        if (it == m_tracks.end()) return;

        if (_fadeOut > 0.0f)
            it->second.voice->FadeOut(_fadeOut);
        else
            it->second.voice->Stop();

        // delete after stopped playing
        it->second.removing = true;
    }

    void StaticChannel::StopAll(float _fadeOut)
    {
        for (auto& [key, track] : m_tracks)
        {
            if (_fadeOut > 0.0f)
                track.voice->FadeOut(_fadeOut);
            else
                track.voice->Stop();

            // Remove tack after done playing
            track.removing = true;
        }
    }

    void StaticChannel::Pause(const std::string& _name)
    {
        auto it = m_tracks.find(_name);
        if (it == m_tracks.end()) return;

        it->second.voice->Pause();
    }

    void StaticChannel::PauseAll()
    {
        for (auto& [key, track] : m_tracks)
        {
            track.voice->Pause();
        }
    }

    void StaticChannel::Resume(const std::string& _name)
    {
        auto it = m_tracks.find(_name);
        if (it == m_tracks.end()) return;

        it->second.voice->Resume();
    }

    void StaticChannel::ResumeAll()
    {
        for (auto& [key, track] : m_tracks)
        {
            track.voice->Resume();
        }
    }

    void StaticChannel::FadeInTrack(const std::string& _name, float _duration, bool _loop)
    {
        auto it = m_tracks.find(_name);
        if (it == m_tracks.end()) return;

        it->second.voice->FadeIn(_duration, _loop);
    }

    void StaticChannel::FadeOutTrack(const std::string &_name, float _duration)
    {
        auto it = m_tracks.find(_name);
        if (it == m_tracks.end()) return;

        it->second.voice->FadeOut(_duration);
    }

    void StaticChannel::FadeToTrack(const std::string &_name, float _target, float _duration)
    {
        auto it = m_tracks.find(_name);
        if (it == m_tracks.end()) return;

        it->second.voice->FadeTo(_target, _duration);
    }

    void StaticChannel::FadeInChannel(float _duration, bool _loop)
    {
        for (auto& [key, track] : m_tracks)
        {
            track.voice->FadeIn(_duration, _loop);
        }
    }

    void StaticChannel::FadeOutChannel(float _duration)
    {
        for (auto& [key, track] : m_tracks)
        {
            track.voice->FadeOut(_duration);
        }
    }

    void StaticChannel::FadeToChannel(float _target, float _duration)
    {
        for (auto& [key, track] : m_tracks)
        {
            track.voice->FadeTo(_target, _duration);
        }
    }

    void StaticChannel::SetPitchTrack(const std::string& _name, float _pitch)
    {
        auto it = m_tracks.find(_name);
        if (it == m_tracks.end()) return;

        it->second.voice->SetPitch(std::clamp(_pitch * m_channelPitch, 0.0f, 1.0f));
    }

    void StaticChannel::SetPitchChannel(float _pitch)
    {
        for (auto& [key, track] : m_tracks)
        {
            track.voice->SetPitch(_pitch);
        }

        m_channelPitch = _pitch;
    }

    void StaticChannel::SetPanTrack(const std::string& _name, float _pan)
    {
        auto it = m_tracks.find(_name);
        if (it == m_tracks.end()) return;

        it->second.voice->SetPan(std::clamp(_pan * m_channelPan, 0.0f, 1.0f));
    }

    void StaticChannel::SetPanChannel(float _pan)
    {
        for (auto& [key, track] : m_tracks)
        {
            track.voice->SetPan(_pan);
        }

        m_channelPan = _pan;
    }

    float StaticChannel::GetPanTrack(const std::string& _name) const
    {
        auto it = m_tracks.find(_name);
        if (it == m_tracks.end()) return 0.0f;

        return it->second.voice->GetPan();
    }

    float StaticChannel::GetPanChannel() const
    {
        return m_channelPan;
    }

    void StaticChannel::SetPanModeTrack(const std::string& _name, PanMode _mode)
    {
        auto it = m_tracks.find(_name);
        if (it == m_tracks.end()) return;

        it->second.voice->SetPanMode(_mode);
    }

    void StaticChannel::SetPanModeChannel(PanMode _mode)
    {
        for (auto& [key, track] : m_tracks)
        {
            track.voice->SetPanMode(_mode);
        }

        m_channelPanMode = _mode;
    }

    PanMode StaticChannel::GetPanModeChannel(const std::string& _name) const
    {
        return m_channelPanMode;
    }

    bool StaticChannel::IsPlaying(const std::string& _name) const
    {
        auto it = m_tracks.find(_name);
        if (it != m_tracks.end())
            return it->second.voice->IsPlaying();

        return false;
    }

    bool StaticChannel::IsFinished(const std::string& _name) const
    {
        auto it = m_tracks.find(_name);
        if (it != m_tracks.end())
            return it->second.voice->IsFinished();

        return false;
    }

    void StaticChannel::SetTrackVolume(const std::string& _name, float _vol)
    {
        auto it = m_tracks.find(_name);
        if (it != m_tracks.end())
            it->second.voice->SetVolume(std::clamp(_vol * m_channelVol, 0.0f, 1.0f));
        else
            Warn("[AUDIO - TRACKS] Tried setting music volume to a non existent track");
    }

    float StaticChannel::GetTrackVolume(const std::string& _name) const
    {
        auto it = m_tracks.find(_name);
        if (it != m_tracks.end())
            return it->second.voice->GetVolume();

        return 1.0f;
    }

    void StaticChannel::SetChannelVolume(float _vol)
    {
        for (auto& [name, track] : m_tracks)
        {
            track.voice->SetVolume(_vol);
        }

        m_channelVol = _vol;
    }

    float StaticChannel::GetChannelVolume() const
    {
        return m_channelVol;
    }
}
