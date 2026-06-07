
#include "ManagerAudioTracker.h"

#include "AudioManager.h"
#include "Helpers/Printer.hpp"

namespace RR
{
    ManagerAudioTracker::ManagerAudioTracker()
    = default;

    ManagerAudioTracker::ManagerAudioTracker(AudioManager* _owner, std::string _key, float _vol)
        : m_owner(_owner), m_key(std::move(_key)), m_volume(_vol) {}

    void ManagerAudioTracker::PlayOneShot() const
    {
        PlayOneShot(m_volume);
    }

    void ManagerAudioTracker::PlayOneShot(float _vol) const
    {
        if (!m_owner)
        {
            Warn("[AUDIO - TRACKER] ManagerAudioTracker has no bound manager! Discarding");
            return;
        }
        
        m_owner->PlayOneShot(m_key, _vol);
    }

    void ManagerAudioTracker::SetVolume(float _vol)
    {
        m_volume = _vol;
    }

    float ManagerAudioTracker::GetVolume() const
    {
        return m_volume;
    }

    const std::string& ManagerAudioTracker::GetKey() const
    {
        return m_key;
    }

    bool ManagerAudioTracker::IsValid() const
    {
        return m_owner != nullptr;
    }
}
