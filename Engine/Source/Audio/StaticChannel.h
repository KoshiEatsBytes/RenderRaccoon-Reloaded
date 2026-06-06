
#pragma once
#include <string>
#include <memory>
#include <unordered_map>

#include "StaticAudio.h"

namespace RR
{
    class StaticChannel
    {
    public:
        StaticChannel();
        ~StaticChannel();

        void Update();

        void AddOneShot(const std::string& _name, std::unique_ptr<StaticAudio> _voice, float _vol);
        void Add(const std::string& _name, std::unique_ptr<StaticAudio> _voice, bool _loop, float _fadeIn = 0.0f);

        void Stop(const std::string& _name, float _fadeOut = 0.0f);
        void StopAll(float _fadeOut = 0.0f);

        void Pause(const std::string& _name);
        void PauseAll();

        void Resume(const std::string& _name);
        void ResumeAll();

        void FadeInTrack(const std::string& _name, float _duration, bool _loop = false);
        void FadeOutTrack(const std::string& _name, float _duration);
        void FadeToTrack(const std::string& _name, float _target, float _duration);
        void FadeInChannel(float _duration, bool _loop = false);
        void FadeOutChannel(float _duration);
        void FadeToChannel(float _target, float _duration);

        void SetPitchTrack(const std::string& _name, float _pitch);
        void SetPitchChannel(float _pitch);

        void SetPanTrack(const std::string& _name, float _pan);
        void SetPanChannel(float _pan);
        float GetPanTrack(const std::string& _name) const;
        float GetPanChannel() const;

        void SetPanModeTrack(const std::string& _name, PanMode _mode);
        void SetPanModeChannel(PanMode _mode);
        PanMode GetPanModeChannel(const std::string& _name) const;

        bool IsPlaying(const std::string& _name) const;
        bool IsFinished(const std::string& _name) const;

        void SetTrackVolume(const std::string& _name, float _vol);
        float GetTrackVolume(const std::string& _name) const;

        void SetChannelVolume(float _vol);
        float GetChannelVolume() const;

    private:
        struct Track
        {
            std::unique_ptr<StaticAudio> voice;
            bool removing = false;
        };

        std::unordered_map<std::string, Track> m_tracks;
        std::vector<std::unique_ptr<StaticAudio>> m_oneShots;
        PanMode m_channelPanMode = PanMode::BALANCE;
        float m_channelPitch = 1.0f;
        float m_channelPan = 1.0f;
        float m_channelVol = 1.0f;
    };
}
