
#include "AudioSourceComponent.h"

#include "Helpers/Printer.hpp"
#include "Scene/GameObject.h"

namespace RR
{
    // PUBLIC ----------------------------------------------------------------------------------------------------------

    AudioSourceComponent::AudioSourceComponent()
    = default;

    AudioSourceComponent::AudioSourceComponent(const std::string& _name, const std::shared_ptr<Audio>& _clip)
    {
        RegisterAudio(_name, _clip);
    }

    AudioSourceComponent::~AudioSourceComponent()
    = default;

    void AudioSourceComponent::Update(float _deltaTime)
    {
        auto pos = m_owner->GetWorldPosition();

        for (auto& [name, clip] : m_audioClips)
        {
            // keep it spatial by updating a clip's position with its GO
            // unless its not spatial
            if (clip->IsPlaying() && clip->IsSpatial())
            {
                clip->SetPosition(pos);
            }
        }
    }

    bool AudioSourceComponent::IsPlaying(const std::string &_name)
    {
        if (m_audioClips.contains(_name))
        {
            return m_audioClips[_name]->IsPlaying();
        }
        return false;
    }

    void AudioSourceComponent::Play(const std::string& _name, bool _loop)
    {
        if (!m_enabled) return;

        if (m_audioClips.contains(_name))
        {
            m_audioClips[_name]->Play(_loop);
        }
    }

    void AudioSourceComponent::Stop(const std::string &_name)
    {
        if (m_audioClips.contains(_name))
        {
            m_audioClips[_name]->Stop();
        }
    }

    void AudioSourceComponent::StopAll() const
    {
        for (auto& clip : m_audioClips)
        {
            clip.second->Stop();
        }
    }

    void AudioSourceComponent::LoadAudio(const std::string& _name, const std::string& _path, bool _spatial)
    {
        if (auto clip = Audio::Load(_path, _spatial))
        {
            RegisterAudio(_name, clip);
        }
    }

    void AudioSourceComponent::RegisterAudio(const std::string& _name, const std::shared_ptr<Audio>& _clip)
    {
        if (m_audioClips.contains(_name))
        {
            Warn("[AUDIO - COMPONENT] Tried to add duplicate audio clip! Clip: '", _name, "' already exists.");
            return;
        }

        m_audioClips[_name] = _clip;
    }

    // PROTECTED -------------------------------------------------------------------------------------------------------
    
    void AudioSourceComponent::OnDisable()
    {
        Component::OnDisable();

        StopAll();
    }
}
