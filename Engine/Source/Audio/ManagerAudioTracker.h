
#pragma once
#include <string>

#include "Tracker.h"

namespace RR
{
    class StaticAudio;
    class AudioManager;

    /**
     * @brief A handle to a manager-side 2D sound.
     */
    class ManagerAudioTracker
    {
    public:
        ManagerAudioTracker();
        ManagerAudioTracker(AudioManager* _owner, Tracker<StaticAudio> _tracker);

        void PlayOneShot() const;

        StaticAudio* operator->() const;

        bool IsValid() const;

        explicit operator bool() const;

    private:
        AudioManager* m_owner = nullptr;
        Tracker<StaticAudio> m_tracker;
    };
}
