
#pragma once
#include <memory>

#include "Helpers/Types.h"

namespace RR
{
    struct AudioBufferRef;
    class AudioClip;
    class AudioVoice
    {
    public:
        virtual ~AudioVoice();
        // Prevent copy & move
        AudioVoice(const AudioVoice&)            = delete;
        AudioVoice& operator=(const AudioVoice&) = delete;
        AudioVoice(AudioVoice&&)                 = delete;
        AudioVoice& operator=(AudioVoice&&)      = delete;

        void Play(bool _loop = false);
        void Stop();
        void Pause();
        void Resume();

        void FadeIn(float _duration, bool _loop = false);
        void FadeOut(float _duration);
        void FadeTo(float _target, float _duration);

        void AttachToGroup(maSoundGroup* _group);
        void DetachFromGroup();

        bool IsFinished() const;
        bool IsPaused() const;
        bool IsPlaying() const;
        bool IsInitialized() const;

        void SetPitch(float _pitch);
        float GetPitch() const;

        void SetVolume(float _vol);
        float GetVolume() const;

        uInt GetChannel() const;
        void SetChannel(uInt _channel);

    protected:
        AudioVoice(std::shared_ptr<AudioClip> _clip, maEngine* _engine);

        virtual void SetSpatial(bool _spatial);
        virtual bool IsSpatial() const;

        std::shared_ptr<AudioClip>        m_clip;
        std::unique_ptr<AudioBufferRef>   m_bufferRef;
        std::unique_ptr<maSound>          m_sound;

        maEngine* m_engineRef = nullptr;
        uInt m_channel = 0;
        bool m_initialized = false;
        bool m_paused = false;

    private:
        bool InitFromClip(maEngine* _engine);
        uint64 DurationToFrames(float _seconds) const;
    };
}

