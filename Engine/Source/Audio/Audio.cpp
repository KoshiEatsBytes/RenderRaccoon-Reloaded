
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
        // Important to first destroy sound the decoder
        if (m_sound)
        {
            ma_sound_uninit(GetSound());
        }
        if (m_decoder)
        {
            ma_decoder_uninit(GetDecoder());
        }
    }

    void Audio::Play(bool _loop) const
    {
        if (m_sound)
        {
            ma_sound_start(GetSound());
            auto looping = _loop ? MA_TRUE : MA_FALSE;
            ma_sound_set_looping(GetSound(), looping);
        }
    }

    void Audio::Stop() const
    {
        if (m_sound)
        {
            ma_sound_stop(GetSound());
            ma_sound_seek_to_pcm_frame(GetSound(), 0);
        }
    }

    bool Audio::IsPlaying() const
    {
        if (m_sound)
        {
            return ma_sound_is_playing(GetSound());
        }
        return false;
    }

    void Audio::SetVolume(float _volume) const
    {
        if (m_sound)
        {
            ma_sound_set_volume(GetSound(), std::clamp(_volume, 0.0f, 1.0f));
        }
    }

    float Audio::GetVolume() const
    {
        if (m_sound)
        {
            return ma_sound_get_volume(GetSound());
        }

        return 0.0f;
    }

    void Audio::SetPosition(const vec3& _pos) const
    {
        if (m_sound)
        {
            ma_sound_set_position(GetSound(), _pos.x, _pos.y, _pos.z);
        }
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
        m_spatial = _spatial;
        auto spatial = _spatial ? MA_TRUE : MA_FALSE;
        ma_sound_set_spatialization_enabled(GetSound(), spatial);
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
        auto audioEng = Engine::GetInstance().GetAudioManager().GetAudioEngine();

        if (buffer.empty())
        {
            Warn("[AUDIO - LOADING] FIle not found at path: '", _path, "'");
            return nullptr;
        }

        auto audio = std::make_shared<Audio>();

        audio->m_sound = std::make_unique<maSound>();
        audio->m_buffer = std::move(buffer);
        audio->m_decoder = std::make_unique<maDecoder>();

        auto result = ma_decoder_init_memory(
            audio->m_buffer.data(),
            audio->m_buffer.size(),
            nullptr,
            audio->m_decoder.get()
            );

        if (result != MA_SUCCESS)
        {
            Warn("[AUDIO - LOADING] Audio file at path: '", _path, "' could not be decoded correctly");
            return nullptr;
        }

        result = ma_sound_init_from_data_source(
            audioEng,
            audio->m_decoder.get(),
            0,
            NULL,
            audio->m_sound.get()
            );

        if (result != MA_SUCCESS)
        {
            Warn("[AUDIO - LOADING] Audio file at path: '", _path, "' could not be initialized correctly from data source");
            return nullptr;
        }

        // set spatial sound
        auto spatial = _spatial ? MA_TRUE : MA_FALSE;
        ma_sound_set_spatialization_enabled(audio->m_sound.get(), spatial);
        audio->m_spatial = _spatial;

        return audio;
    }
}
