
#pragma once
#include <GL/glew.h>
#include <chrono>
#include <vector>
#include <array>
#include <cstddef>

#include "ISubSystem.h"
#include "Helpers/TypeInfo.h"
#include "Benchmark/BenchmarkData.h"
#include "Helpers/Types.h"

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
        void RequestStartLogging(const RunInfo& _runInfo, int _iFrames = 10);
        void RequestStopLogging();
        void RequestDiscard();
        bool IsLogging() const;

        // outside logging
        void RecordSceneMetrics(float _simTime, const vec3& _pos, float _coverage);

        // Per frame engine loop calls
        void BeginFrame(float _deltaTime);
        void BeginGpuTimer();
        void EndGpuTimer();
        void EndFrame();

        BenchmarkRun GetLastRunData() const;

    protected:
        bool Init() override;
        void Destroy() override;

    private:
        void BeginLogging();
        void FinishLogging();
        void DiscardLogging();
        void WriteCSV();

        // 96 mb allocated for benchmark samples, that's a 6-minute run at 8000 FPS
        static constexpr sizeT kSampleSize = 3000000;
        static constexpr int   kRing = 4;
        static constexpr int   kResultPrecision = 7;

        // log toggles
        bool m_logging          = false;
        bool m_startRequested   = false;
        bool m_stopRequested    = false;
        bool m_discardRequested = false;
        bool m_completed        = false;

        // run start metrics
        bool m_captureThisFrame    = false;
        int  m_warmUpFramesPending = 0;
        int  m_warmUpFramesLeft    = 0;

        // scene statistics
        float m_currSimTime  = 0.0f;
        float m_currCoverage = 0.0f;
        vec3  m_currPos      {0.0f};

        // Frame-time
        std::chrono::steady_clock::time_point m_cpuStart;
        float m_frameTimeMs = 0.0f;

        // GPU timer query ring - async
        std::array<GLuint, kRing> m_gpuQueries    {};
        std::array<sizeT, kRing>  m_sampleForSlot {};
        std::array<bool, kRing>   m_slotPending   {};
        long long m_frameCounter = 0;

        // Run info
        FrameStats m_frameStats;
        RunInfo    m_runInfo;

        // Raw samples
        std::vector<FrameSample> m_samples;
    };
}
