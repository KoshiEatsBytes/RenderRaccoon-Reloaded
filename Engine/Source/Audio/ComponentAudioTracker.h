
#pragma once
#include <memory>

#include "Tracker.h"


namespace RR
{
    class AudioSourceComponent;
    class SpatialAudio;

    /**
     * @brief A handle to a component-owned spatial voice.
     */
    class ComponentAudioTracker
    {
    public:
        ComponentAudioTracker();
        ComponentAudioTracker(AudioSourceComponent* _owner,
                              std::weak_ptr<bool> _ownerAlive,
                              Tracker<SpatialAudio> _tracker);

        void PlayOneShot() const;

        SpatialAudio* operator->() const;

        bool IsValid() const;

        explicit operator bool() const;

    private:
        bool OwnerAlive() const;

        AudioSourceComponent* m_owner = nullptr;
        std::weak_ptr<bool>   m_ownerAlive;
        Tracker<SpatialAudio> m_tracker;
    };
}
