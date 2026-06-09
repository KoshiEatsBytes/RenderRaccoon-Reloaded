
#include "BenchmarkSubSystem.h"
#include "Helpers/Printer.hpp"

namespace RR
{
    // PUBLIC ----------------------------------------------------------------------------------------------------------

    BenchmarkSubSystem::BenchmarkSubSystem()
    = default;

    BenchmarkSubSystem::~BenchmarkSubSystem()
    = default;

    bool BenchmarkSubSystem::Init()
    {
        m_samples.reserve(120000);

        const std::vector<FrameSample> test = {
            {10}, {10}, {10}, {10}, {10}, {10}, {10}, {10}, {10}, {100}
        };
        const FrameStats s = ComputeStats(test);
        Log("[BENCHMARK] self-test  avg=", s.avgFrameTimeMs, "ms  fps=", s.avgFps,
            "  min=", s.minFrameTimeMs, "  max=", s.maxFrameTimeMs,
            "  1%low=", s.low1Ms, "  stutters=", s.stutterCount);

        return true;
    }

    void BenchmarkSubSystem::Update(float _deltaTime)
    {
    }

    void BenchmarkSubSystem::Destroy()
    {
        ISubSystem::Destroy();
    }
}
