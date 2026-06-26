
#include <miniaudio.h>
#include <algorithm>
#include <cctype>

#include "AudioManager.h"
#include "Engine.h"
#include "Voice/AudioClip.h"
#include "Helpers/Printer.hpp"

namespace RR
{
    // LIFECYCLE -------------------------------------------------------------------------------------------------------

    AudioManager::AudioManager()
    {
        m_audioEngine = std::make_unique<maEngine>();
    }

    AudioManager::~AudioManager()
    {
        m_channels.clear();

        if (m_initialized)
        {
            ma_engine_uninit(GetAudioEngine());
        }
    }

    bool AudioManager::Init(uInt _channelCount, uInt _fallbackChannel)
    {
        if (_channelCount < 1)
        {
            Error("[AUDIO - INIT] Tried initializing audio manager without any channels!");
            return false;
        }

        if (_fallbackChannel >= _channelCount) // channels start at 0
        {
            Error("[AUDIO - INIT] Tried setting fallback audio channel to channel out of index, defaulting to first");
            _fallbackChannel = 0;
        }

        if (ma_engine_init(nullptr, GetAudioEngine()) != MA_SUCCESS)
        {
            Error("[AUDIO - INIT] MiniAudio Engine has failed to initialize!");
            return false;
        }


        for (uInt id = 0; id < _channelCount; ++id)
        {
            if (!m_channels[id].Init(m_audioEngine.get()))
            {
                Error("[AUDIO - INIT] An audio channel has failed to initialize!");
                return false;
            }
        }

        m_initialized = true;
        m_channelCount = _channelCount;
        m_fallbackChannel = _fallbackChannel;

        ScanAudioAssets();
        return true;
    }

    void AudioManager::Update(float)
    {
        for (auto& [id, channel] : m_channels)
        {
            channel.Update();
        }
    }

    void AudioManager::UnloadAll()
    {
        for (auto& [id, channel] : m_channels)
        {
            channel.Clear();
        }
        m_clipCache.clear();
    }

    // CHANNEL BINDING -------------------------------------------------------------------------------------------------

    void AudioManager::BindTrack(const std::string& _key, uInt _channel)
    {
        if (_channel >= m_channelCount)
        {
            Warn("[AUDIO - BIND] Channel index out of scope! Allocate more channels at init");
            return;
        }

        m_keyToChannel[_key] = _channel;
    }

    uInt AudioManager::ResolveChannel(const std::string& _key, uInt _fallback) const
    {
        if (_fallback >= m_channelCount)
        {
            Warn("[AUDIO - RESOLVE] Fallback channel out of scope! fallback-ing on main channel");
            return 0;
        }

        auto it = m_keyToChannel.find(_key);
        return it != m_keyToChannel.end() ? it->second : _fallback;
    }

    // 2D PLAYBACK -----------------------------------------------------------------------------------------------------

    void AudioManager::PlayOneShot(const Tracker<StaticAudio>& _tracker)
    {
        PlayOneShot(_tracker.GetKey());
    }

    void AudioManager::PlayOneShot(const std::string& _key, float _vol)
    {
        const uInt channel = ResolveChannel(_key, m_fallbackChannel);

        // clone the chached voice so settings stick
        auto prototype = GetOrCreateVoice<StaticAudio>(_key, channel);
        if (!prototype) return;

        auto clone = CreateVoiceShared<StaticAudio>(_key, channel);
        if (!clone) return;

        clone->CloneSettings(*prototype);
        if (_vol != 1.0f) clone->SetVolume(prototype->GetVolume() * _vol);

        // Route into the bus, then play; the channel reaps it once finished.
        m_channels[channel].AddOneShot(clone);
        clone->Play(false);
    }

    void AudioManager::PlayManaged(const std::string& _key, bool _loop, float _fadeIn)
    {
        const uInt channel = ResolveChannel(_key, m_fallbackChannel);

        // Reuse the cached voice if present so its settings persist across plays.
        auto voice = GetOrCreateVoice<StaticAudio>(_key, channel);
        if (!voice) return;

        if (_fadeIn > 0.0f)
            voice->FadeIn(_fadeIn, _loop);
        else
            voice->Play(_loop);
    }

    void AudioManager::StopManaged(const std::string& _key, float _fadeOut)
    {
        auto channel = ResolveChannel(_key, m_fallbackChannel);
        m_channels[channel].Stop(_key, _fadeOut);
    }

    void AudioManager::CrossFade(const std::string& _from, const std::string& _to, float _seconds, bool _loop)
    {
        StopManaged(_from, _seconds);
        PlayManaged(_to, _loop, _seconds);
    }

    void AudioManager::StopChannel(uInt _channel, float _fadeOut)
    {
        if (_channel >= m_channelCount)
        {
            Warn("[AUDIO - STOP] Channel index out of scope! Allocate more channels at init");
            return;
        }

        m_channels[_channel].StopAll(_fadeOut);
    }

    // TRACKERS --------------------------------------------------------------------------------------------------------

    ManagerAudioTracker AudioManager::GetStatic(const std::string& _key)
    {
        return {this, GetTrack<StaticAudio>(_key)};
    }

    std::shared_ptr<SpatialAudio> AudioManager::CreateSpatial(const std::string& _key, uInt _channel)
    {
        if (_channel >= m_channelCount)
        {
            Warn("[AUDIO - SPATIAL] channel out of scope, reverting to fallback");
            _channel = m_fallbackChannel;
        }

        // Deliberately does NOT bind the key globally
        auto voice = CreateVoiceShared<SpatialAudio>(_key, _channel);
        if (voice)
        {
            // routes bus to component
            voice->AttachToGroup(m_channels[_channel].GetGroup());
        }
        return voice;
    }

    // VOLUME ----------------------------------------------------------------------------------------------------------

    void AudioManager::SetChannelVolume(uInt _channel, float _vol)
    {
        if (_channel >= m_channelCount)
        {
            Warn("[AUDIO - SET CHANNEL VOLUME] Channel index out of scope! Allocate more channels at init");
            return;
        }

        m_channels[_channel].SetVolume(_vol);
    }

    float AudioManager::GetChannelVolume(uInt _channel) const
    {
        if (_channel >= m_channelCount)
        {
            Warn("[AUDIO - GET CHANNEL VOLUME] Channel index out of scope! Allocate more channels at init");
            return 1.0f;
        }

        auto it = m_channels.find(_channel);
        return it != m_channels.end() ? it->second.GetVolume() : 1.0f;
    }

    void AudioManager::SetMasterVolume(float _vol)
    {
        if (m_initialized)
        {
            ma_engine_set_volume(m_audioEngine.get(), std::clamp(_vol, 0.0f, 1.0f));
        }
    }

    // LISTENER --------------------------------------------------------------------------------------------------------

    void AudioManager::SetListenerParams(const vec3& _pos, const vec3& _dir, const vec3& _up) const
    {
        if (!m_audioEngine)
        {
            Warn("[AUDIO MANAGER - LISTENER] Tried setting listener params with invalid engine");
            return;
        }
        ma_engine_listener_set_position (GetAudioEngine(), 0, _pos.x, _pos.y, _pos.z);
        ma_engine_listener_set_direction(GetAudioEngine(), 0, _dir.x, _dir.y, _dir.z);
        ma_engine_listener_set_world_up (GetAudioEngine(), 0, _up.x,  _up.y,  _up.z);
    }

    void AudioManager::SetListenerVelocity(const vec3& _vel) const
    {
        if (m_audioEngine) ma_engine_listener_set_velocity(GetAudioEngine(), 0, _vel.x, _vel.y, _vel.z);
    }

    void AudioManager::SetListenerDirection(const vec3& _dir) const
    {
        if (m_audioEngine) ma_engine_listener_set_direction(GetAudioEngine(), 0, _dir.x, _dir.y, _dir.z);
    }

    void AudioManager::SetListenerWorldUp(const vec3& _up) const
    {
        if (m_audioEngine) ma_engine_listener_set_world_up(GetAudioEngine(), 0, _up.x, _up.y, _up.z);
    }

    void AudioManager::SetListenerPosition(const vec3& _pos) const
    {
        if (m_audioEngine) ma_engine_listener_set_position(GetAudioEngine(), 0, _pos.x, _pos.y, _pos.z);
    }

    maEngine* AudioManager::GetAudioEngine() const
    {
        return m_audioEngine.get();
    }

    // CLIP CACHE (FLYWEIGHT) ------------------------------------------------------------------------------------------

    std::shared_ptr<AudioClip> AudioManager::GetClip(const std::string& _key)
    {
        // already decoded & still referenced
        if (auto it = m_clipCache.find(_key); it != m_clipCache.end())
        {
            if (auto cached = it->second.lock()) return cached;
        }

        auto itPath = m_keyToPath.find(_key);
        if (itPath == m_keyToPath.end())
        {
            Warn("[AUDIO - GET] Requested unknown audio key '", _key, "'");
            return nullptr;
        }

        auto clip = DecodeClip(itPath->second);
        if (clip)
        {
            m_clipCache[_key] = clip;
            return clip;
        }

        Warn("[AUDIO - GET] Audio key '", _key, "' could not be decoded");
        return nullptr;
    }

    std::shared_ptr<AudioClip> AudioManager::DecodeClip(const std::string& _relativePath)
    {
        auto rawBytes = Engine::GetInstance().GetFileSystem().LoadAssetFile(_relativePath);
        if (rawBytes.empty())
        {
            Warn("[AUDIO - CLIP] Audio clip empty or not found at: '", _relativePath, "'");
            return nullptr;
        }

        ma_decoder_config config = ma_decoder_config_init(ma_format_f32,
            0, ma_engine_get_sample_rate(m_audioEngine.get()));

        ma_decoder decoder;
        if (ma_decoder_init_memory(rawBytes.data(), rawBytes.size(), &config, &decoder) != MA_SUCCESS)
        {
            Warn("[AUDIO - CLIP] Failed to decode clip at: '", _relativePath, "'");
            return nullptr;
        }

        const uInt32 channels   = decoder.outputChannels;
        const uInt32 sampleRate = decoder.outputSampleRate;

        ma_uint64 frameCount = 0;
        ma_decoder_get_length_in_pcm_frames(&decoder, &frameCount);

        std::vector<float> pcm(frameCount * channels);
        ma_uint64 framesRead = 0;
        ma_decoder_read_pcm_frames(&decoder, pcm.data(), frameCount, &framesRead);
        ma_decoder_uninit(&decoder);

        if (framesRead == 0)
        {
            Warn("[AUDIO - CLIP] Loaded clip with 0 frames at: '", _relativePath, "', discarding");
            return nullptr;
        }
        pcm.resize(framesRead * channels);

        return std::make_shared<AudioClip>(std::move(pcm), framesRead, channels, sampleRate);
    }

    void AudioManager::ScanAudioAssets()
    {
        auto& fileSystem = Engine::GetInstance().GetFileSystem();
        const auto files = fileSystem.ListAssetFiles("Audio", {".wav", ".mp3", ".flac", ".ogg"});

        for (const auto& relToRoot : files)
        {
            const std::string key = DeriveKey(relToRoot.lexically_relative("Audio"));

            if (m_keyToPath.contains(key))
            {
                Warn("[AUDIO - SCAN] Duplicate key '", key, "' from '", relToRoot.generic_string(),
                     "' — already mapped to '", m_keyToPath[key], "'. Rename to disambiguate.");
                continue;
            }
            m_keyToPath[key] = relToRoot.generic_string();
        }

        Log("[AUDIO] Registered ", m_keyToPath.size(), " audio assets");
    }

    void AudioManager::AppendWords(const std::string& _segment, std::vector<std::string>& _out)
    {
        std::string word;
        for (char ch : _segment)
        {
            if (ch == '_' || ch == '-' || ch == ' ')
            {
                if (!word.empty())
                {
                    _out.push_back(word);
                    word.clear();
                }
            }
            else
            {
                word += ch;
            }
        }
        if (!word.empty()) _out.push_back(word);
    }

    std::string AudioManager::DeriveKey(const fSysPath& _relToAudio)
    {
        std::vector<std::string> words;
        for (const auto& segment : _relToAudio.parent_path())
        {
            AppendWords(segment.string(), words);
        }
        AppendWords(_relToAudio.stem().string(), words);

        std::string key;
        for (auto& word : words)
        {
            word[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(word[0])));
            key += word;
        }
        if (!key.empty())
        {
            key[0] = static_cast<char>(std::tolower(static_cast<unsigned char>(key[0])));
        }
        return key;
    }
}
