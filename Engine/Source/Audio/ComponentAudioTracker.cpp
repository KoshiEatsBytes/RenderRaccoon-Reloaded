
#include "ComponentAudioTracker.h"
#include "Helpers/Printer.hpp"
#include "Scene/Components/AudioSourceComponent.h"

namespace RR
{
    ComponentAudioTracker::ComponentAudioTracker()
    = default;

    ComponentAudioTracker::ComponentAudioTracker(AudioSourceComponent* _owner,
        std::weak_ptr<bool> _ownerAlive, Tracker<SpatialAudio> _tracker)
        : m_owner(_owner), m_ownerAlive(std::move(_ownerAlive)), m_tracker(std::move(_tracker)) {}

    void ComponentAudioTracker::PlayOneShot() const
    {
        if (OwnerAlive())
        {
            m_owner->PlayOneShot(m_tracker);
            return;
        }

        Warn("[AUDIO - TRACKER] ComponentAudioTracker has outlived its component! Discarding");
    }

    SpatialAudio* ComponentAudioTracker::operator->() const
    {
        if (OwnerAlive()) return m_tracker.Lock().get();

        Warn("[AUDIO - TRACKER] ComponentAudioTracker has outlived its component! Discarding");
        return nullptr;
    }

    bool ComponentAudioTracker::IsValid() const
    {
        return OwnerAlive() && static_cast<bool>(m_tracker);
    }

    ComponentAudioTracker::operator bool() const
    {
        return IsValid();
    }

    bool ComponentAudioTracker::OwnerAlive() const
    {
        // Check if owner is still alive
        return !m_ownerAlive.expired() && m_owner != nullptr;
    }
}
