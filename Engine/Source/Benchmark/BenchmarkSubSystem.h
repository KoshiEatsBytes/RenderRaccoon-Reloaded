
#pragma once
#include <GL/glew.h>
#include <chrono>
#include <vector>
#include <array>
#include <cstddef>

#include "ISubSystem.h"
#include "Helpers/TypeInfo.h"
#include "Benchmark/FrameStats.h"

namespace RR
{
    class BenchmarkSubSystem : public ISubSystem
    {
    public:
        SUBSYSTEM(BenchmarkSubSystem, RR::ISubSystem)

        BenchmarkSubSystem();
        ~BenchmarkSubSystem() override;

        // Lifecycle
        void InitGpuQueries();
        void DestroyGpuQueries();

        // Logging requests
        void RequestStartLogging();
        void RequestStopLogging();
        bool IsLogging() const;

        // Per frame engine loop calls
        void BeginFrame(float _deltaTime);
        void BeginGpuTimer();
        void EndGpuTimer();
        void EndFrame();

    protected:
        bool Init() override;
        void Destroy() override;

    private:
        void BeginLogging();
        void FinishLogging();

        // 60 mb allocated for benchmark samples, that's a 10-minute run at 8000 FPS
        static constexpr sizeT kSampleSize = 5000000;
        static constexpr int   kRing = 4;
        static constexpr float kWarmUpSeconds = 1.0f;

        // log toggles
        bool m_logging        = false;
        bool m_startRequested = false;
        bool m_stopRequested  = false;

        bool  m_captureThisFrame  = false;
        float m_warmUpSecondsLeft = 0.0f;

        // Frame-time
        std::chrono::steady_clock::time_point m_cpuStart;
        float m_frameTimeMs = 0.0f;

        // GPU timer query ring - async
        std::array<GLuint, kRing> m_gpuQueries    {};
        std::array<sizeT, kRing>  m_sampleForSlot {};
        std::array<bool, kRing>   m_slotPending   {};
        long long m_frameCounter = 0;

        // Raw samples
        std::vector<FrameSample> m_samples;
    };
}
