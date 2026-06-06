
#include <miniaudio.h>

#include "AudioManager.h"

#include "Engine.h"
#include "Voice/AudioClip.h"
#include "SpatialAudio.h"
#include "StaticAudio.h"

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
        ScanAudioAssets();
        return true;
    }

    std::shared_ptr<AudioClip> AudioManager::GetClip(const std::string &_key)
    {
        // Check if clip has already been loaded
        auto it = m_clipCache.find(_key);
        if (it != m_clipCache.end())
        {
            if (auto cached = it->second.lock())
            {
                return cached;
            }
        }

        // If has not beed loaded - check if it exists in dir
        auto itPath = m_keyToPath.find(_key);
        if (itPath == m_keyToPath.end())
        {
            Warn("[AUDIO - GET] Requested unknown audio file '", _key, "'");
            return nullptr;
        }

        // create it and load it
        auto clip = DecodeClip(itPath->second);
        if (clip)
        {
            m_clipCache[_key] = clip;
            return clip;
        }

        Warn("[AUDIO - GET] Requested audio file '", _key, "' could not be decoded.");
        return nullptr;
    }

    std::unique_ptr<SpatialAudio> AudioManager::CreateSpatial(const std::string& _key)
    {
        // returns new spatial source
        if (auto clip = GetClip(_key))
        {
            return std::make_unique<SpatialAudio>(clip, m_audioEngine.get());
        }
        return nullptr;
    }

    std::unique_ptr<StaticAudio> AudioManager::CreateStatic(const std::string& _key)
    {
        // returns new static source
        if (auto clip = GetClip(_key))
        {
            return std::make_unique<StaticAudio>(clip, m_audioEngine.get());
        }
        return nullptr;
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

    maEngine* AudioManager::GetAudioEngine() const
    {
        return m_audioEngine.get();
    }

    // PRIVATE ---------------------------------------------------------------------------------------------------------

    std::shared_ptr<AudioClip> AudioManager::DecodeClip(const std::string& _relativePath)
    {
        auto rawBytes = Engine::GetInstance().GetFileSystem().LoadAssetFile(_relativePath);
        if (rawBytes.empty())
        {
            Warn("[AUDIO - CLIP] Audio clip Empty or not found at: '", _relativePath, "'");
            return nullptr;
        }

        // configure clip decoder
        ma_decoder_config config = ma_decoder_config_init(
            ma_format_f32,
            0, //native channels
            ma_engine_get_sample_rate(m_audioEngine.get())
            );

        ma_decoder decoder;
        auto result = ma_decoder_init_memory(
            rawBytes.data(),
            rawBytes.size(),
            &config, &decoder
            );

        // let user know if decodition explodesss
        if (result != MA_SUCCESS)
        {
            Warn("[AUDIO - CLIP] Failed to decode clip at: '", _relativePath, "'");
            return nullptr;
        }

        const uint32 channels   = decoder.outputChannels;
        const uint32 sampleRate = decoder.outputSampleRate;

        ma_uint64 frameCount = 0;
        ma_decoder_get_length_in_pcm_frames(&decoder, &frameCount);

        // read data to vector then delete decoder
        std::vector<float> pcm(frameCount * channels);
        ma_uint64 framesRead = 0;
        ma_decoder_read_pcm_frames(&decoder, pcm.data(), frameCount, &framesRead);
        ma_decoder_uninit(&decoder);

        if (framesRead == 0)
        {
            Warn("[AUDIO - CLIP] Loaded clip with 0 frames at: '", _relativePath, "' discarding");
            return nullptr;
        }
        pcm.resize(framesRead * channels);

        // if valid, ship!
        return std::make_shared<AudioClip>(std::move(pcm), framesRead, channels, sampleRate);
    }

    void AudioManager::ScanAudioAssets()
    {
        auto& fileSystem = Engine::GetInstance().GetFileSystem();
        const auto files = fileSystem.ListAssetFiles(
            "Audio", {".wav", ".mp3", ".flac", ".ogg"});

        // Iterate over files globbed
        for (const auto& relToRoot : files)
        {
            const std::string key = DeriveKey(relToRoot.lexically_relative("Audio"));

            // Prevent duplicate naming files
            if (m_keyToPath.contains(key))
            {
                Warn("[AUDIO - SCAN] Duplicate key '", key, "' from '", relToRoot.generic_string(),
                 "' — already mapped to '", m_keyToPath[key], "'. Rename to distinguish");
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
            if (ch=='_' || ch=='-' || ch==' ')
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

        for (auto& currWord : words)
        {
            currWord[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(currWord[0])));
            key += currWord;
        }

        if (!key.empty())
        {
            key[0] = static_cast<char>(std::tolower(static_cast<unsigned char>(key[0])));
        }

        return key;
    }
}




















