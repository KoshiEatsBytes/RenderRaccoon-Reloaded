
#pragma once
#include <vector>
#include <cstddef>
#include <cstdint>
#include <string>

namespace RR
{
    using sizeT = std::size_t;

    // One frame raw timings
    struct FrameSample
    {
        // frame ms
        float frameTimeMs = 0.0f;
        float cpuMs       = 0.0f;
        float gpuMs       = 0.0f;

        // scene data
        float simTime  = 0.0f;
        float posX     = 0.0f;
        float posY     = 0.0f;
        float posZ     = 0.0f;
        float coverage = 0.0f;
    };

    // Saved as part of the CSV header, is-per-run
    struct RunInfo
    {
        std::string name          = "";
        std::string scene         = "unknown";
        std::string config        = "debug";
        uint32_t    seed          = 0;
        bool        completed     = false;
        bool        deterministic = false;

        // Optimizations
        bool lod         = false;
        bool aggregation = false;
        bool greedy      = false;
        bool async       = false;
        bool scheduling  = false;

        // Render config
        int   renderDistance = 16;
        float warmUpSeconds  = 0.0f;
        int   steadyDraws    = 0;   // draw calls after loading complete

        // Test platform
        std::string  gpuName   = "unknown";
        std::string  cpuName   = "unknown";
        unsigned int coreCount = 0;
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
        sizeT gpuSampleCount = 0;

        // Fps data
        float avgFps  = 0.0f;
        float low10Pc = 0.0f;
        float low5Pc  = 0.0f;
        float low1Pc  = 0.0f;
        float low01Pc = 0.0f;

        // Coverage
        float coverageAvg    = 0.0f;
        float coverageLow1Pc = 0.0f;
        float coverageMin    = 0.0f;

        // Stutter
        unsigned int stutterCount = 0;
    };

    struct BenchmarkRun
    {
        RunInfo                  info;
        FrameStats               stats;
        std::vector<FrameSample> samples;
    };

    FrameStats ComputeStats(const std::vector<FrameSample>& _samples);
    // Klein et al. 2023: 4–12 ms variation is perceptible
    constexpr float kStutterDeltaMs = 6.0f;
}













