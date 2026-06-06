
#include <miniaudio.h>
#include <algorithm>

#include "AudioVoice.h"
#include "AudioClip.h"
#include "Helpers/Printer.hpp"

namespace RR
{
    struct AudioBufferRef { ma_audio_buffer_ref ref; };

    // PROTECTED -------------------------------------------------------------------------------------------------------

    AudioVoice::AudioVoice(std::shared_ptr<AudioClip> _clip, maEngine* _engine)
        : m_clip(std::move(_clip))
    {
        if (!m_clip)
        {
            Warn("[AUDIO - VOICE] Tried creating audio voice with INVALID audio clip");
            return;
        }

        if (!_engine)
        {
            Warn("[AUDIO - VOICE] Tried creating audio voice with INVALID Audio Engine");
            return;
        }

        m_initialized = InitFromClip(_engine);
    }

    void AudioVoice::SetSpatial(bool _spatial)
    {
        if (!m_initialized)
        {
            Warn("[AUDIO - SPATIAL] Tried setting spatial mode of an uninitialized audio file");
            return;
        }

        auto spatial = _spatial ? MA_TRUE : MA_FALSE;
        ma_sound_set_spatialization_enabled(m_sound.get(), spatial);
    }

    bool AudioVoice::IsSpatial() const
    {
        if (!m_initialized) return false;

        return ma_sound_is_spatialization_enabled(m_sound.get());
    }

    // PUBLIC ----------------------------------------------------------------------------------------------------------

    AudioVoice::~AudioVoice()
    {
        if (m_sound)
        {
            ma_sound_uninit(m_sound.get());
        }
        if (m_bufferRef)
        {
            ma_audio_buffer_ref_uninit(&m_bufferRef->ref);
        }
    }

    void AudioVoice::Play(bool _loop)
    {
        if (!m_initialized)
        {
            Warn("[AUDIO - PLAY] Tried playing an uninitialized audio file");
            return;
        }

        //sets the frame to 0 and cancel scheduled sto, if any
        ma_sound_reset_stop_time_and_fade(m_sound.get());
        ma_sound_seek_to_pcm_frame(m_sound.get(), 0);
        ma_sound_set_looping(m_sound.get(), _loop ? MA_TRUE : MA_FALSE);
        ma_sound_start(m_sound.get());
    }

    void AudioVoice::Stop()
    {
        if (!m_initialized)
        {
            Warn("[AUDIO - STOP] Tried stopping an uninitialized audio file");
            return;
        }

        // stops and set frame cursor back to 0
        ma_sound_stop(m_sound.get());
        ma_sound_seek_to_pcm_frame(m_sound.get(), 0);
    }

    void AudioVoice::Pause()
    {
        if (!m_initialized)
        {
            Warn("[AUDIO - PAUSE] Tried paused an uninitialized audio file");
            return;
        }

        m_paused = true;
        ma_sound_stop(m_sound.get());
    }

    void AudioVoice::Resume()
    {
        if (!m_initialized)
        {
            Warn("[AUDIO - RESUME] Tried resuming an uninitialized audio file");
            return;
        }

        if (!m_paused) return;

        m_paused = false;
        ma_sound_start(m_sound.get());
    }

    void AudioVoice::FadeIn(float _duration, bool _loop)
    {
        if (!m_initialized)
        {
            Warn("[AUDIO - FADE] Tried fading an uninitialized audio file");
            return;
        }

        // If stopped restart from the beginning
        if (!ma_sound_is_playing(m_sound.get())) Play(_loop);

        // Apply fade
        const ma_uint64 fadeFrames = DurationToFrames(_duration);
        ma_sound_set_fade_in_pcm_frames(m_sound.get(), 0.0f, 1.0f, fadeFrames);
    }

    void AudioVoice::FadeOut(float _duration)
    {
        if (!m_initialized)
        {
            Warn("[AUDIO - FADE] Tried fading an uninitialized audio file");
            return;
        }

        FadeTo(0.0f, _duration);

        // stops automatically when fade finishes
        const ma_uint64 fadeFrames = DurationToFrames(_duration);
        const ma_uint64 stopFrame = ma_engine_get_time_in_pcm_frames(m_engineRef) + fadeFrames;
        ma_sound_set_stop_time_in_pcm_frames(m_sound.get(), stopFrame);
    }

    void AudioVoice::FadeTo(const float _target, float _duration)
    {
        if (!m_initialized)
        {
            Warn("[AUDIO - FADE] Tried fading an uninitialized audio file");
            return;
        }

        const ma_uint64 fadeFrames = DurationToFrames(_duration);
        ma_sound_set_fade_in_pcm_frames(m_sound.get(), -1.0f, _target, fadeFrames);
    }

    bool AudioVoice::IsFinished() const
    {
        return m_initialized && ma_sound_at_end(m_sound.get());
    }

    bool AudioVoice::IsPlaying() const
    {
        return m_initialized && ma_sound_is_playing(m_sound.get());
    }

    bool AudioVoice::IsInitialized() const
    {
        return m_initialized;
    }

    void AudioVoice::SetPitch(float _pitch)
    {
        if (!m_initialized)
        {
            Warn("[AUDIO - PITCH] Tried setting pitch to an uninitialized audio file");
            return;
        }

        // pitch must not be 0
        ma_sound_set_pitch(m_sound.get(), std::max(_pitch, 0.01f));
    }

    float AudioVoice::GetPitch() const
    {
        if (!m_initialized) return 1.0f;
        return ma_sound_get_pitch(m_sound.get());
    }

    /**
     * @brief WARN: Resets fades when called.
     */
    void AudioVoice::SetVolume(float _vol)
    {
        if (!m_initialized)
        {
            Warn("[AUDIO - VOL] Tried setting volume to an uninitialized audio file");
            return;
        }

        ma_sound_reset_fade(m_sound.get());
        ma_sound_set_volume(m_sound.get(), std::clamp(_vol, 0.0f, 1.0f));
        ma_sound_set_fade_in_pcm_frames(m_sound.get(), 1.0f, 1.0f, 0);
    }

    float AudioVoice::GetVolume() const
    {
        if (!m_initialized) return 0.0f;
        float fadeVol = ma_sound_get_current_fade_volume(m_sound.get());
        return ma_sound_get_volume(m_sound.get()) * fadeVol;
    }

    // PRIVATE ---------------------------------------------------------------------------------------------------------

    bool AudioVoice::InitFromClip(maEngine* _engine)
    {
        m_bufferRef = std::make_unique<AudioBufferRef>();
        ma_result result = ma_audio_buffer_ref_init(
            ma_format_f32,
            m_clip->GetChannels(),
            m_clip->GetPcm().data(),
            m_clip->GetFrameCount(),
            &m_bufferRef->ref
            );

        if (result != MA_SUCCESS)
        {
            Warn("[AUDIO - VOICE] buffer ref initialization has failed, clip has not been initialized");
            return false;
        }

        // Set rate as init does not take it
        m_bufferRef->ref.sampleRate = m_clip->GetSampleRate();

        m_sound = std::make_unique<maSound>();
        result = ma_sound_init_from_data_source(
            _engine, m_bufferRef.get(), 0, nullptr, m_sound.get());

        if (result != MA_SUCCESS)
        {
            Warn("[AUDIO - VOICE] Sound initialization has failed");
            ma_audio_buffer_ref_uninit(&m_bufferRef->ref);
            m_bufferRef.reset();
            m_sound.reset();
            return false;
        }

        // All good
        m_engineRef = _engine;
        return true;
    }

    uint64 AudioVoice::DurationToFrames(float _seconds) const
    {
        return static_cast<uint64>(_seconds * static_cast<float>(ma_engine_get_sample_rate(m_engineRef)));
    }
}






















