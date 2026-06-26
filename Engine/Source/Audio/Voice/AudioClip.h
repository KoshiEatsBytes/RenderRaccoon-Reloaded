
#pragma once
#include <vector>
#include <utility>

#include "Helpers/Types.h"

namespace RR
{
    class AudioClip
    {
    public:
        AudioClip(std::vector<float> _pcm, uInt64 _frames, uInt32 _channels, uInt32 _rate)
            : m_pcm(std::move(_pcm)), m_frameCount(_frames), m_channels(_channels), m_sampleRate(_rate) {
        }

        ~AudioClip() = default;

        const std::vector<float>& GetPcm() const {
            return m_pcm;
        }

        uInt64 GetFrameCount() const {
            return m_frameCount;
        }

        uInt32 GetChannels() const {
            return m_channels;
        }

        uInt32 GetSampleRate() const {
            return m_sampleRate;
        }

        bool IsMono() const {
            // Gate for spatial sounds
            return m_channels == 1;
        }

    private:
        std::vector<float> m_pcm; // Pulse code modulation, basically fully uncompressed file
        uInt64 m_frameCount  = 0;
        uInt32 m_channels    = 0;
        uInt32 m_sampleRate  = 0;
    };
}


