
#pragma once
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

#include "Helpers/Common.h"
#include "AudioChannel.h"
#include "Tracker.h"
#include "Helpers/Printer.hpp"

namespace RR
{
    class AudioClip;
    class AudioManager
    {
        AudioManager();
        ~AudioManager();

    public:
        friend class Engine;

        // Delete copy and move
        AudioManager(const AudioManager&) = delete;
        AudioManager& operator=(const AudioManager&) = delete;
        AudioManager(AudioManager&&) noexcept = delete;
        AudioManager& operator=(AudioManager&&) noexcept = delete;

        // CHANNEL BINDING
        // A key's home channel, if not set explicitly will be defaulted to general
        void BindTrack(const std::string& _key, uInt _channel);

        // PLAYBACK
        void PlayOneShot(const std::string& _key, float _vol = 1.0f);
        void PlayManaged(const std::string& _key, bool _loop = true, float _fadeIn = 0.0f);
        void StopManaged(const std::string& _key, float _fadeOut = 0.0f);
        void CrossFade  (const std::string& _from, const std::string& _to, float _seconds = 1.0f, bool _loop = true);
        void StopChannel(uInt _channel, float _fadeOut = 0.0f);

        // TRACKERS
        Tracker<StaticAudio>  GetStatic (const std::string& _key);
        Tracker<StaticAudio>  GetOneShot(const std::string& _key);
        Tracker<SpatialAudio> GetSpatial(const std::string& _key);

        // VOLUME
        void  SetChannelVolume(uInt _channel, float _vol);
        float GetChannelVolume(uInt _channel) const;
        void  SetMasterVolume(float _vol);

        // LISTENER
        void SetListenerParams(const vec3& _pos, const vec3& _dir, const vec3& _up) const;
        void SetListenerDirection(const vec3& _dir) const;
        void SetListenerWorldUp(const vec3& _up) const;
        void SetListenerPosition(const vec3& _pos) const;

        std::shared_ptr<AudioClip> GetClip(const std::string& _key);
        maEngine* GetAudioEngine() const;

    private:
        bool Init(uInt _channelCount, uInt _fallbackChannel);
        void Update(float _deltaTime);
        void ScanAudioAssets();

        std::shared_ptr<AudioClip> DecodeClip(const std::string& _relativePath);
        static void AppendWords(const std::string& _segment, std::vector<std::string>& _out);
        static std::string DeriveKey(const fSysPath& _relToAudio);

        uInt ResolveChannel(const std::string& _key, uInt _fallback = 0) const;

        // flyweight & routing
        std::unordered_map<std::string, std::string>              m_keyToPath;
        std::unordered_map<std::string, std::weak_ptr<AudioClip>> m_clipCache;
        std::unordered_map<std::string, uInt>                     m_keyToChannel;
        std::unordered_map<uInt, AudioChannel>                    m_channels;

        std::unique_ptr<maEngine> m_audioEngine;
        bool m_initialized = false;
        uInt m_fallbackChannel = 0;
        uInt m_channelCount = 0;

        // TEMPLATES -------------------------------------------------------------------------------------------------------

        template<typename T>
        std::shared_ptr<T> CreateVoiceShared(const std::string& _key)
        {
            auto clip = GetClip(_key);
            return clip ? std::make_shared<T>(clip, m_audioEngine.get()) : nullptr;
        }

    public:

        template<typename E> requires std::is_enum_v<E>
        void BindTrack(const std::string& _key, E _channel)
        {
            BindTrack(_key, static_cast<uInt>(_channel));
        }

        template<typename T>
        Tracker<T> GetTrack(const std::string& _key, uInt _channel)
        {
            if (_channel >= m_channelCount)
            {
                Warn("[AUDIO - RESOLVE] Channel index out of scope! fallback on main channel");
                return {};
            }

            m_keyToChannel[_key] = _channel;

            AudioChannel& channel = m_channels[_channel];
            std::shared_ptr<T> live = std::static_pointer_cast<T>(channel.Find(_key));

            auto revive = [this, &channel, _key]() -> std::shared_ptr<T>
            {
                auto voice = CreateVoiceShared<T>(_key);
                if (voice) channel.Add(_key, voice);
                return voice;
            };
            return Tracker<T>(live, std::move(revive));
        }
    };
}
