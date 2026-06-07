
#include "ManagerAudioTracker.h"

#include "AudioManager.h"

namespace RR
{
    ManagerAudioTracker::ManagerAudioTracker()
    = default;

    ManagerAudioTracker::ManagerAudioTracker(AudioManager *_owner, Tracker<StaticAudio> _tracker)
        : m_owner(_owner), m_tracker(std::move(_tracker)) {
    }

    void ManagerAudioTracker::PlayOneShot() const
    {
        m_owner->PlayOneShot(m_tracker);
    }

    StaticAudio* ManagerAudioTracker::operator->() const
    {
        return m_tracker.Lock().get();
    }

    bool ManagerAudioTracker::IsValid() const
    {
        return static_cast<bool>(m_tracker);
    }

    ManagerAudioTracker::operator bool() const
    {
        return IsValid();
    }
}
