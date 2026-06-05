
#include <miniaudio.h>
#include "Audio.h"

#include "Engine.h"

namespace RR
{
    // public ----------------------------------------------------------------------------------------------------------

    Audio::Audio()
    = default;

    Audio::~Audio()
    {
        // Important to first destroy sound then decoder
        if (m_sound)
        {
            ma_sound_uninit(GetSound());
        }
        if (m_decoder)
        {
            ma_decoder_uninit(GetDecoder());
        }
    }

    void Audio::Play(bool _loop)
    {
        if (m_sound)
        {
            ma_sound_start(GetSound());
            auto looping = _loop ? MA_TRUE : MA_FALSE;
            ma_sound_set_looping(GetSound(), looping);
            return;
        }

        Warn("[AUDIO - PLAY] Tried playing an uninitialized audio file");
    }

    void Audio::Stop()
    {
        if (m_sound)
        {
            ma_sound_stop(GetSound());
            ma_sound_seek_to_pcm_frame(GetSound(), 0);
            return;
        }

        Warn("[AUDIO - STOP] Tried stopping an uninitialized audio file");
    }

    bool Audio::IsPlaying() const
    {
        if (m_sound)
        {
            return ma_sound_is_playing(GetSound());
        }

        return false;
    }

    void Audio::SetVolume(float _volume)
    {
        if (m_sound)
        {
            ma_sound_set_volume(GetSound(), std::clamp(_volume, 0.0f, 1.0f));
            return;
        }

        Warn("[AUDIO - VOLUME] Tried setting volume of an uninitialized audio file");
    }

    float Audio::GetVolume() const
    {
        if (m_sound)
        {
            return ma_sound_get_volume(GetSound());
        }

        return 0.0f;
    }

    void Audio::SetPosition(const vec3& _pos)
    {
        if (m_sound)
        {
            ma_sound_set_position(GetSound(), _pos.x, _pos.y, _pos.z);
            return;
        }


        Warn("[AUDIO - POSITION] Tried setting position of an uninitialized audio file");
    }

    vec3 Audio::GetPosition() const
    {
        if (m_sound)
        {
            // you discover something new about C++ every day
            const auto [x, y, z] = ma_sound_get_position(GetSound());
            return vec3 {x, y, z};
        }
        return vec3{0.f};
    }

    bool Audio::IsSpatial() const
    {
        return m_spatial;
    }

    void Audio::SetSpatial(bool _spatial)
    {
        if (m_sound)
        {
            m_spatial = _spatial;
            auto spatial = _spatial ? MA_TRUE : MA_FALSE;
            ma_sound_set_spatialization_enabled(GetSound(), spatial);
            return;
        }

        Warn("[AUDIO - SPATIAL] Tried setting spatial mode of an uninitialized audio file");
    }

    maSound* Audio::GetSound() const
    {
        return m_sound.get();
    }

    maDecoder* Audio::GetDecoder() const
    {
        return m_decoder.get();
    }

    std::shared_ptr<Audio> Audio::Load(const std::string& _path, bool _spatial)
    {
        auto buffer = Engine::GetInstance().GetFileSystem().LoadAssetFile(_path);
        if (buffer.empty())
        {
            Warn("[AUDIO - LOADING] File not found at path: '", _path, "'");
            return nullptr;
        }

        auto* audioEng = Engine::GetInstance().GetAudioManager().GetAudioEngine();
        auto  audio    = std::make_shared<Audio>();

        // buffer must outlive decoder
        audio->m_buffer = std::move(buffer);

        // Init on local, save only on success
        auto decoder = std::make_unique<maDecoder>();
        if (ma_decoder_init_memory(audio->m_buffer.data(), audio->m_buffer.size(),
                                   nullptr, decoder.get()) != MA_SUCCESS)
        {
            Warn("[AUDIO - LOADING] Could not decode: '", _path, "'");
            return nullptr;
        }
        audio->m_decoder = std::move(decoder);

        // Same for sound
        auto sound = std::make_unique<maSound>();
        if (ma_sound_init_from_data_source(audioEng, audio->m_decoder.get(),
                                           0, nullptr, sound.get()) != MA_SUCCESS)
        {
            Warn("[AUDIO - LOADING] Could not init sound from data source: '", _path, "'");
            return nullptr;
        }
        audio->m_sound = std::move(sound);

        ma_sound_set_spatialization_enabled(audio->m_sound.get(), _spatial ? MA_TRUE : MA_FALSE);
        audio->m_spatial = _spatial;
        return audio;
    }
}
