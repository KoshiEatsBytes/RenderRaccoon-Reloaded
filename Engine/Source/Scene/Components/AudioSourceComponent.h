
#pragma once
#include "Scene/Component.h"
#include "Audio/Audio.h"

namespace RR
{
    class AudioSourceComponent : public Component
    {
    public:
        COMPONENT(AudioSourceComponent)

        AudioSourceComponent();
        AudioSourceComponent(const std::string& _name, const std::shared_ptr<Audio>& _clip);
        ~AudioSourceComponent() override;

        void Update(float _deltaTime) override;

        bool IsPlaying(const std::string& _name);
        void Play(const std::string& _name, bool _loop = false);
        void Stop(const std::string& _name);
        void StopAll() const;

        void LoadAudio(const std::string& _name, const std::string& _path, bool _spatial = true);
        void RegisterAudio(const std::string& _name, const std::shared_ptr<Audio>& _clip);

    protected:
        void OnDisable() override;

    private:
        std::unordered_map<std::string, std::shared_ptr<Audio>> m_audioClips;
    };
}
