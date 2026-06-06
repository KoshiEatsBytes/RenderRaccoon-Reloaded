
#pragma once
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

#include "Helpers/Common.h"

namespace RR
{
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

        bool Init();

        // Voice factory and clip cache
        std::shared_ptr<AudioClip>    GetClip(const std::string& _key);
        std::unique_ptr<SpatialAudio> CreateSpatial(const std::string& _key);
        std::unique_ptr<StaticAudio>  CreateStatic (const std::string& _key);

        // Listener
        void SetListenerParams(const vec3& _pos, const vec3& _dir, const vec3& _up) const;
        void SetListenerDirection(const vec3& _dir) const;
        void SetListenerWorldUp(const vec3& _up) const;
        void SetListenerPosition(const vec3& _pos) const;

        maEngine* GetAudioEngine() const;

    private:
        std::shared_ptr<AudioClip> DecodeClip(const std::string& _relativePath);

        void ScanAudioAssets();
        static void AppendWords(const std::string& _segment, std::vector<std::string>& _out);
        static std::string DeriveKey(const fSysPath& _relToAudio);

        // flyweight system
        std::unordered_map<std::string, std::string> m_keyToPath; // catalog
        std::unordered_map<std::string, std::weak_ptr<AudioClip>> m_clipCache;

        bool m_initialized = false;
        std::unique_ptr<maEngine> m_audioEngine;
    };
}















