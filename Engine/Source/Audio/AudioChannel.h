
#pragma once
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

#include "Helpers/Types.h" // maEngine, maSoundGroup

namespace RR
{
    class AudioVoice;

    /**
     * @brief A mixing bus, owns a miniaudio sound group plus the voices routed into it
     *
     * Channel controls are applied to the whole group by the the mixer
     * Per-track control is deliberately NOT here, grab a Tracker for that.
     */
    class AudioChannel
    {
    public:
        AudioChannel();
        ~AudioChannel();

        // deleted move & copy
        AudioChannel(const AudioChannel&) = delete;
        AudioChannel& operator=(const AudioChannel&) = delete;
        AudioChannel(AudioChannel&&) noexcept = delete;
        AudioChannel& operator=(AudioChannel&&) noexcept = delete;

        bool Init(maEngine* _engine);
        void Update();

        // Collection lifecycle
        void Add(const std::string& _name, std::shared_ptr<AudioVoice> _voice);
        void AddOneShot(std::shared_ptr<AudioVoice> _voice);
        void Stop(const std::string& _name, float _fadeOut = 0.0f);
        void StopAll(float _fadeOut = 0.0f);
        void Clear();

        // Lookup
        std::shared_ptr<AudioVoice> Find(const std::string& _name) const;
        bool Contains(const std::string& _name) const;

        // Bus controls
        void  SetVolume(float _vol);
        float GetVolume() const;
        void  SetPitch(float _pitch);
        float GetPitch() const;
        void  SetPan(float _pan);
        float GetPan() const;

        maSoundGroup* GetGroup() const;

    private:
        std::unique_ptr<maSoundGroup> m_group;
        std::unordered_map<std::string, std::shared_ptr<AudioVoice>> m_tracks;
        std::vector<std::shared_ptr<AudioVoice>> m_oneShots;

        float m_volume = 1.0f;
        float m_pitch  = 1.0f;
        float m_pan    = 0.0f;
        bool  m_initialized = false;
    };
}
