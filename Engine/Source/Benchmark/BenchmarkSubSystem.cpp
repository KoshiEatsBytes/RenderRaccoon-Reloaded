
#include "BenchmarkSubSystem.h"
#include "Helpers/Printer.hpp"

namespace RR
{
    // PUBLIC ----------------------------------------------------------------------------------------------------------

    BenchmarkSubSystem::BenchmarkSubSystem()
    = default;

    BenchmarkSubSystem::~BenchmarkSubSystem()
    = default;

    void BenchmarkSubSystem::InitGpuQueries()
    {
        // Generates diagnostics GL queries
        glGenQueries(kRing, m_gpuQueries.data());
        m_slotPending.fill(false);
    }

    void BenchmarkSubSystem::DestroyGpuQueries()
    {
        glDeleteQueries(kRing, m_gpuQueries.data());
    }

    void BenchmarkSubSystem::RequestStartLogging()
    {
        if (m_logging)
        {
            Warn("[BENCHMARK - LOG] Requested benchmark start when already logging, Discarding.",
                 "Data will likely be invalidated by this otuput stream");
            return;
        }

        m_startRequested = true;
    }

    void BenchmarkSubSystem::RequestStopLogging()
    {
        if (!m_logging)
        {
            Warn("[BENCHMARK - LOG] Requested benchmark stop when already stopped. Discarding");
            return;
        }

        m_stopRequested = true;
    }

    bool BenchmarkSubSystem::IsLogging() const
    {
        return m_logging;
    }

    // Must be run at the very beginning of a frame
    void BenchmarkSubSystem::BeginFrame(float _deltaTime)
    {
        // Process stops
        if (m_stopRequested)
        {
            m_stopRequested  = false;
            FinishLogging();

            Log("[BENCHMARK - STOP] Stopping benchmarking on current scene");
        }
        // process starts
        if (m_startRequested)
        {
            m_startRequested = false;

            Log("[BENCHMARK - START] Starting benchmarking on current scene ",
                kWarmUpSeconds, " seconds before logging beings.");

            BeginLogging();
        }

        // if not logging, stop frame from being captured
        if (!m_logging)
        {
            m_captureThisFrame = false;
            return;
        }

        // tick down warm up before logging
        if (m_warmUpSecondsLeft > 0.0f)
        {
            m_warmUpSecondsLeft -= _deltaTime;
            m_captureThisFrame   = false;
            return;
        }

        m_captureThisFrame = true;
        m_frameTimeMs = _deltaTime * 1000.0f;
        m_cpuStart    = std::chrono::steady_clock::now();
    }

    void BenchmarkSubSystem::BeginGpuTimer()
    {
        if (!m_captureThisFrame) return;
        const int slot = static_cast<int>(m_frameCounter % kRing);

        // Read slot PREVIOUS result, issued kRing frame ago
        // gpu runs behind, so it must be mapped correctly
        // add blocking in case gpu LAGS behind on low-end hardware
        if (m_slotPending[slot])
        {
            GLint available = GL_FALSE;
            glGetQueryObjectiv(m_gpuQueries[slot], GL_QUERY_RESULT_AVAILABLE, &available);

            if (available)
            {
                GLuint64 nanoSeconds = 0;
                glGetQueryObjectui64v(m_gpuQueries[slot], GL_QUERY_RESULT, &nanoSeconds);
                m_samples[m_sampleForSlot[slot]].gpuMs = static_cast<float>(nanoSeconds) / 1.0e6f; // magic numer for ns -> ms
            }

            // If GPU is lagging ( > kRing frames) - drop this gpu sample
            m_slotPending[slot] = false;
        }

        glBeginQuery(GL_TIME_ELAPSED, m_gpuQueries[slot]);
    }

    void BenchmarkSubSystem::EndGpuTimer()
    {
        if (!m_captureThisFrame) return;
        glEndQuery(GL_TIME_ELAPSED);
    }

    void BenchmarkSubSystem::EndFrame()
    {
        if (!m_captureThisFrame) return;

        // log cpu work this frame
        const auto  cpuEnd = std::chrono::steady_clock::now();
        const float cpuMs  = std::chrono::duration<float, std::milli>(cpuEnd - m_cpuStart).count();

        // store this frame, gpu will be updated in kRing frames
        m_samples.push_back(FrameSample{m_frameTimeMs, cpuMs, 0.0f});

        // prepare slot for GPU
        const int slot = static_cast<int>(m_frameCounter % kRing);
        m_sampleForSlot[slot] = m_samples.size() - 1;
        m_slotPending[slot] = true;

        m_frameCounter++;
    }

    // PROTECTED -------------------------------------------------------------------------------------------------------

    bool BenchmarkSubSystem::Init()
    {
        InitGpuQueries();
        return true;
    }

    void BenchmarkSubSystem::Destroy()
    {
        if (m_logging)
        {
            FinishLogging();
            Log("[BENCHMARK] Application closed before benchmark asked to log, saving before closing...");
        }

        DestroyGpuQueries();
    }

    // PRIVATE ---------------------------------------------------------------------------------------------------------

    void BenchmarkSubSystem::BeginLogging()
    {
        // Reserve sample size, don't allow mid run reallocations
        m_samples.clear();
        m_samples.reserve(kSampleSize);
        m_slotPending.fill(false);

        // Begin benching after delay
        m_frameCounter      = 0;
        m_warmUpSecondsLeft = kWarmUpSeconds;
        m_logging           = true;
    }

    void BenchmarkSubSystem::FinishLogging()
    {
        if (!m_logging) return;
        m_logging = false;

        // GPU runs behind, so stop and save GL query results that have not
        // been output yet
        for (int slot = 0; slot < kRing; slot++)
        {
            if (!m_slotPending[slot]) continue;

            // Save buffered frames, this is blocking but fine at the end of logging.
            GLuint64 nanoSeconds = 0;
            glGetQueryObjectui64v(m_gpuQueries[slot], GL_QUERY_RESULT, &nanoSeconds);
            m_samples[m_sampleForSlot[slot]].gpuMs = static_cast<float>(nanoSeconds) / 1.0e6f;
            m_slotPending[slot] = false;
        }

        // Temporary
        const FrameStats sample = ComputeStats(m_samples);
        Log("[BENCHMARK] ", static_cast<int>(sample.frameCount), " frames  avg=", sample.avgFrameTimeMs,
            "ms (", sample.avgFps, " fps)  1%low=", sample.low1Ms, "  0.1%low=", sample.low01Ms,
            "  max=", sample.maxFrameTimeMs, "  stutters=", sample.stutterCount,
            "  cpu=", sample.avgCpuMs, "  gpu=", sample.avgGpuMs);
    }
}



















