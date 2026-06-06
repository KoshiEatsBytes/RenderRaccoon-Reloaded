
#pragma once
#include <vector>
#include <utility>

#include "Helpers/Types.h"

namespace RR
{
    class AudioClip
    {
    public:
        AudioClip(std::vector<float> _pcm, uint64 _frames, uint32 _channels, uint32 _rate)
            : m_pcm(std::move(_pcm)), m_frameCount(_frames), m_channels(_channels), m_sampleRate(_rate) {
        }

        ~AudioClip() = default;

        const std::vector<float>& GetPcm() const {
            return m_pcm;
        }

        uint64 GetFrameCount() const {
            return m_frameCount;
        }

        uint32 GetChannels() const {
            return m_channels;
        }

        uint32 GetSampleRate() const {
            return m_sampleRate;
        }

        bool IsMono() const {
            // Gate for spatial sounds
            return m_channels == 1;
        }

    private:
        std::vector<float> m_pcm; // Pulse code modulation, basically fully uncompressed file
        uint64 m_frameCount  = 0;
        uint32 m_channels    = 0;
        uint32 m_sampleRate  = 0;
    };
}


