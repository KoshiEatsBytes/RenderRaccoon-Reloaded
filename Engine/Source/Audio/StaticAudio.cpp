
#include "StaticAudio.h"

#include "miniaudio.h"
#include "Helpers/Printer.hpp"

namespace RR
{
    // LOCAL -----------------------------------------------------------------------------------------------------------

    static ma_pan_mode ToMa(PanMode _model)
    {
        switch (_model)
            {
                case PanMode::BALANCE:
                    return ma_pan_mode_balance;

                case PanMode::PAN:
                    return ma_pan_mode_pan;

                default:
                    return ma_pan_mode_balance;
            }
    }

    // PUBLIC ----------------------------------------------------------------------------------------------------------

    StaticAudio::StaticAudio(std::shared_ptr<AudioClip> _clip, maEngine *_engine)
        : AudioVoice(std::move(_clip), _engine)
    {
        if (m_initialized) AudioVoice::SetSpatial(false);
    }

    void StaticAudio::SetPan(float _pan)
    {
        if (!m_initialized)
        {
            Warn("[STATIC AUDIO - PAN] Tried setting pan to an uninitialized audio file");
            return;
        }

        ma_sound_set_pan(m_sound.get(), std::clamp(_pan, -1.0f, 1.0f));
    }

    void StaticAudio::SetPanMode(PanMode _mode)
    {
        if (!m_initialized)
        {
            Warn("[STATIC AUDIO - PAN MODE] Tried setting pan mode to an uninitialized audio file");
            return;
        }

        ma_sound_set_pan_mode(m_sound.get(), ToMa(_mode));
    }

    float StaticAudio::GetPan() const
    {
        if (m_initialized) return ma_sound_get_pan(m_sound.get());
        return 0.0f;
    }
}