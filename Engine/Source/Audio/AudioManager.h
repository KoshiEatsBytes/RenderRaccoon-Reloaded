
#pragma once
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

#include "StaticChannel.h"
#include "Helpers/Common.h"

namespace RR
{
    enum class StaticChannelIndex : int
    {
        EFFECTS,
        MUSIC,

        GENERAL = 0
    };


    class AudioClip;
    class SpatialAudio;
    class StaticAudio;
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

        // 2D sounds
        void PlayOneShot   (const std::string& _name, float _vol = 1.0f);

        // Music
        void PlayMusic     (const std::string& _name, bool _loop = true, float _fadeIn = 1.0f);
        void StopMusic     (const std::string& _name, float _fadeOut = 1.0f);
        void StopAllMusic  (float _fadeOut = 0.0f);
        void CrossFadeMusic(const std::string& _from, const std::string& _to, float _seconds, bool _loop = false);
        void SetMusicVolume(const std::string& _name, float _vol);


        // Managed
        void PlayManaged     (const std::string& _name, bool _loop = true, float _fadeIn = 1.0f);
        void StopManaged     (const std::string& _name, float _fadeOut = 1.0f);
        void StopAllManaged  (float _fadeOut = 0.0f);
        void CrossFadeManaged(const std::string& _from, const std::string& _to, float _seconds, bool _loop = false);

        // Voice factory and clip cache
        std::shared_ptr<AudioClip>    GetClip(const std::string& _name);
        std::unique_ptr<SpatialAudio> CreateSpatial(const std::string& _name);
        std::unique_ptr<StaticAudio>  CreateStatic (const std::string& _name);

        // Volume
        void SetMasterVolume(float _vol);
        void SetOneShotVolume(float _vol);

        // Listener
        void SetListenerParams(const vec3& _pos, const vec3& _dir, const vec3& _up) const;
        void SetListenerDirection(const vec3& _dir) const;
        void SetListenerWorldUp(const vec3& _up) const;
        void SetListenerPosition(const vec3& _pos) const;

        maEngine* GetAudioEngine() const;

    private:
        bool Init();
        void ScanAudioAssets();
        void Update(float _deltaTime);

        std::shared_ptr<AudioClip> DecodeClip(const std::string& _relativePath);
        static void AppendWords(const std::string& _segment, std::vector<std::string>& _out);
        static std::string DeriveKey(const fSysPath& _relToAudio);

        // flyweight system
        std::unordered_map<std::string, std::string> m_keyToPath; // catalog
        std::unordered_map<std::string, std::weak_ptr<AudioClip>> m_clipCache;
        std::vector<std::unique_ptr<StaticAudio>> m_oneShots;
        std::unordered_map<int, StaticChannel> m_channels;

        std::unique_ptr<maEngine> m_audioEngine;
        bool m_initialized = false;
        float m_oneShotVolume = 1.0f;
    };
}















