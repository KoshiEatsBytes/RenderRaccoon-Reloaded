
#pragma once
#include "Voice/AudioVoice.h"

namespace RR
{
    enum class PanMode
    {
        BALANCE,
        PAN
    };

    class StaticAudio : public AudioVoice
    {
    public:
        StaticAudio(std::shared_ptr<AudioClip> _clip, maEngine* _engine);

        void SetPan(float _pan);
        void SetPanMode(PanMode _mode);
        float GetPan() const;
    };
}
