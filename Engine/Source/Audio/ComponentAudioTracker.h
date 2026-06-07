
#pragma once
#include "Tracker.h"


namespace RR
{
    class AudioSourceComponent;
    class SpatialAudio;
    class ComponentAudioTracker
    {
    public:
        ComponentAudioTracker();
        ComponentAudioTracker(AudioSourceComponent* _owner, Tracker<SpatialAudio> _tracker);

        void PlayOneShot() const;
        SpatialAudio* operator->() const;

    private:
        AudioSourceComponent* m_owner = nullptr;
        Tracker<SpatialAudio> m_tracker;
    };
}
