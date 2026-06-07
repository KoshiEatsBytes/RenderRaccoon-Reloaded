
#include "ComponentAudioTracker.h"
#include "Helpers/Printer.hpp"
#include "Scene/Components/AudioSourceComponent.h"

namespace RR
{
    ComponentAudioTracker::ComponentAudioTracker()
    = default;

    ComponentAudioTracker::ComponentAudioTracker(AudioSourceComponent* _owner, Tracker<SpatialAudio> _tracker)
        : m_owner(_owner), m_tracker(std::move(_tracker)) {}

    void ComponentAudioTracker::PlayOneShot() const
    {
        if (m_owner)
        {
            m_owner->PlayOneShot(m_tracker);
            return;
        }

        Warn("[AUDIO - TRACKER] OneShotTracker has outlived its component! Discarding");
    }

    SpatialAudio* ComponentAudioTracker::operator->() const
    {
        if (m_owner) return m_tracker.Lock().get();

        Warn("[AUDIO - TRACKER] OneShotTracker has outlived its component! Discarding");
        return nullptr;
    }
}
