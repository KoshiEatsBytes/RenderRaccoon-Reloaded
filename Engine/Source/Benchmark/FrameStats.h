
#pragma once
#include <vector>
#include <cstddef>

namespace RR
{
    using sizeT = std::size_t;

    // One frame raw timings
    struct FrameSample
    {
        float frameTimeMs = 0.0f;
        float cpuMs       = 0.0f;
        float gpuMs       = 0.0f;
    };

    // Data container for run data constructed from raw samples
    struct FrameStats
    {
        sizeT frameCount = 0;

        // Frame times
        float avgFrameTimeMs = 0.0f;
        float minFrameTimeMs = 0.0f;
        float maxFrameTimeMs = 0.0f;
        float stdDeviationMs = 0.0f;

        // Diagnostic - which is the bottleneck
        float avgCpuMs = 0.0f;
        float avgGpuMs = 0.0f;

        // Fps data
        float avgFps  = 0.0f;
        float low10Ms = 0.0f;
        float low5Ms  = 0.0f;
        float low1Ms  = 0.0f;
        float low01Ms = 0.0f;

        // Stutter
        int stutterCount = 0;
    };

    FrameStats ComputeStats(const std::vector<FrameSample>& _samples);
    constexpr float stutterThreshold = 2.0f;
}













