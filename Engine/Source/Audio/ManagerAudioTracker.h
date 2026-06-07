
#pragma once
#include <string>

namespace RR
{
    class AudioManager;

    /**
     * @brief A handle to a manager-side 2D one-shot key.
     */
    class ManagerAudioTracker
    {
    public:
        ManagerAudioTracker();
        ManagerAudioTracker(AudioManager* _owner, std::string _key, float _vol = 1.0f);

        // Fire a fresh, overlapping one-shot — at the sticky volume, or an override.
        void PlayOneShot() const;
        void PlayOneShot(float _vol) const;

        // Sticky volume applied to subsequent PlayOneShot() calls.
        void  SetVolume(float _vol);
        float GetVolume() const;

        const std::string& GetKey() const;

        bool IsValid() const;
        explicit operator bool() const { return IsValid(); }

    private:
        AudioManager* m_owner = nullptr;
        std::string   m_key;
        float         m_volume = 1.0f;
    };
}
