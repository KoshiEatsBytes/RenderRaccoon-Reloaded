
#include <fstream>
#include <iomanip>
#include <ctime>

#include "BenchmarkSubSystem.h"
#include "Helpers/Printer.hpp"
#include "FileSystem/FileSystem.h"
#include "Engine.h"

namespace RR
{
    // STATIC ----------------------------------------------------------------------------------------------------------

    // Unique filenames to distinguish runs.
    static std::string MakeTimestampName()
    {
        const std::time_t time = std::time(nullptr);
        std::tm tm{};
#if defined(_WIN32)
        localtime_s(&tm, &time);
#else
        localtime_r(&time, &tm);
#endif
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d_%H-%M-%S", &tm);
        return buf;
    }

    // Make a run name safe to embed in a filename 
    // Copy paste from stack overflow, idk whats going on here
    static std::string SanitizeForFilename(const std::string& _s)
    {
        std::string out;
        for (char c : _s)
        {
            const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-';
            out += ok ? c : '_';
        }
        return out;
    }

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

    // Request benchmark start, specify amount of frames to skip before log, default 10
    void BenchmarkSubSystem::RequestStartLogging(const RunInfo& _runInfo, int _iFrames)
    {
        if (m_logging)
        {
            Warn("[BENCHMARK - LOG] Requested benchmark start when already logging, Discarding.",
                 "Data will likely be invalidated by this output stream");
            return;
        }

        m_warmUpFramesPending = _iFrames;
        m_startRequested = true;
        m_runInfo = _runInfo;
        m_runInfo.completed = false;

        // Save device info
        m_runInfo.gpuName   = m_appData.gpuName;
        m_runInfo.cpuName   = m_appData.cpuName;
        m_runInfo.coreCount = m_appData.coreCount;
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

    void BenchmarkSubSystem::RequestDiscard()
    {
        m_startRequested = false;
        m_stopRequested  = false;

        if (m_logging) m_discardRequested = true;
    }

    bool BenchmarkSubSystem::IsLogging() const
    {
        return m_logging;
    }

    void BenchmarkSubSystem::RecordSceneMetrics(float _simTime, const vec3 &_pos, float _coverage)
    {
        m_currSimTime  = _simTime;
        m_currPos      = _pos;
        m_currCoverage = _coverage;
    }

    // Must be run at the very beginning of a frame
    void BenchmarkSubSystem::BeginFrame(float _deltaTime)
    {
        // Process stops
        if (m_stopRequested)
        {
            m_stopRequested     = false;
            m_completed         = true;
            FinishLogging();

            Log("[BENCHMARK - STOP] Stopping benchmarking on current scene");
        }
        // process discard
        else if (m_discardRequested)
        {
            m_discardRequested = false;
            DiscardLogging();
            Warn("[BENCHMARK - DISCARD] Run invalidated (pause/quit/fail) — no file written");
        }

        // process starts
        if (m_startRequested)
        {
            m_startRequested = false;

            Log("[BENCHMARK - START] Starting benchmarking on current scene...");

            BeginLogging();
        }

        // if not logging, stop frame from being captured
        if (!m_logging)
        {
            m_captureThisFrame = false;
            return;
        }

        // tick down warm up frames before logging
        if (m_warmUpFramesLeft > 0)
        {
            --m_warmUpFramesLeft;
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
        FrameSample sample;
        sample.frameTimeMs = m_frameTimeMs;
        sample.cpuMs       = cpuMs;
        sample.gpuMs       = 0.0f;
        sample.simTime     = m_currSimTime;
        sample.posX        = m_currPos.x;
        sample.posY        = m_currPos.y;
        sample.posZ        = m_currPos.z;
        sample.coverage    = m_currCoverage;

        m_samples.push_back(sample);

        // prepare slot for GPU
        const int slot = static_cast<int>(m_frameCounter % kRing);
        m_sampleForSlot[slot] = m_samples.size() - 1;
        m_slotPending[slot] = true;

        m_frameCounter++;
    }

    BenchmarkRun BenchmarkSubSystem::GetLastRunData() const
    {
        return {m_runInfo, m_frameStats, m_samples};
    }

    // PROTECTED -------------------------------------------------------------------------------------------------------

    bool BenchmarkSubSystem::Init()
    {
        InitGpuQueries();
        // Reserve once at start
        m_samples.reserve(kSampleSize);
        return true;
    }

    void BenchmarkSubSystem::Destroy()
    {
        if (m_logging)
        {
            DiscardLogging();
            Log("[BENCHMARK] Application closed before before requesting benchmark stop, discarding data!");
        }

        DestroyGpuQueries();
    }

    // PRIVATE ---------------------------------------------------------------------------------------------------------

    void BenchmarkSubSystem::BeginLogging()
    {
        // Clear per run
        m_samples.clear();
        m_slotPending.fill(false);

        // Begin benching after delay
        m_frameCounter      = 0;
        m_warmUpFramesLeft  = m_warmUpFramesPending;
        m_logging           = true;
        m_completed         = false;
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

        // Saves run data
        m_frameStats = ComputeStats(m_samples);
        WriteCSV();
    }

    void BenchmarkSubSystem::DiscardLogging()
    {
        if (!m_logging) return;

        m_logging = false;
        m_completed = false;

        // abandon already present frame results
        m_samples.clear();
        m_slotPending.fill(false);
    }

    void BenchmarkSubSystem::WriteCSV()
    {
        // get save path from FileSys
        const std::string prefix  = SanitizeForFilename(m_runInfo.name.empty() ? "bench" : m_runInfo.name);
        const std::string relPath = "Benchmarks/" + prefix + "_" + MakeTimestampName() + ".csv";
        const std::string fullPath = Engine::GetInstance().GetFileSystem().GetOutputFolder().string() + relPath;

        std::ofstream out = Engine::GetInstance().GetFileSystem().OpenOutputFile(relPath);
        if (!out) return;

        // preserve sub microsecond in the log
        out << std::setprecision(kResultPrecision);

#ifdef NDEBUG
        const char* config = "Release";
#else
        const char* config = "Debug";
#endif

        // Update run info with last details
        m_runInfo.completed = m_completed;
        m_runInfo.config = config;

        // Save run details first
        out << "# config="         << config                   << "\n"
            << "# frames="         << m_samples.size()         << "\n"
            << "# completed="      << m_runInfo.completed      << "\n"
            << "# name="           << m_runInfo.name           << "\n"
            << "# scene="          << m_runInfo.scene          << "\n"
            << "# seed="           << m_runInfo.seed           << "\n"
            << "# renderDistance=" << m_runInfo.renderDistance << "\n"
            << "# warmUpSeconds="  << m_runInfo.warmUpSeconds  << "\n"
            << "# steadyDraws="    << m_runInfo.steadyDraws    << "\n"
            << "# steadyTris="     << m_runInfo.steadyTris     << "\n"
            << "# lod="            << m_runInfo.lod            << "\n"
            << "# aggregation="    << m_runInfo.aggregation    << "\n"
            << "# greedy="         << m_runInfo.greedy         << "\n"
            << "# async="          << m_runInfo.async          << "\n"
            << "# scheduling="     << m_runInfo.scheduling     << "\n"
            << "# deterministic="  << m_runInfo.deterministic  << "\n"
            << "# gpu="            << m_runInfo.gpuName        << "\n"
            << "# cpu="            << m_runInfo.cpuName        << "\n"
            << "# cores="          << m_runInfo.coreCount      << "\n"
            << "# physicalCores="  << m_runInfo.physicalCores  << "\n"
            << "# workerThreads="  << m_runInfo.workerThreads  << "\n";

        // Saves the raw frame data
        out << "frameIdx,frameTimeMs,cpuMs,gpuMs,simTime,posX,posY,posZ,coverage\n";
        for (sizeT i = 0; i < m_samples.size(); i++)
        {
            const FrameSample& sample = m_samples[i];
            out << i                  << ','
                << sample.frameTimeMs << ','
                << sample.cpuMs       << ','
                << sample.gpuMs       << ','
                << sample.simTime     << ','
                << sample.posX        << ','
                << sample.posY        << ','
                << sample.posZ        << ','
                << sample.coverage    << '\n';
        }

        // Save to file
        out.flush();
        if (!out)
        {
            Error("[BENCHMARK] CSV write failed — file may be incomplete");
            return;
        }

        Success("[BENCHMARK] Benchmark of scene: '", m_runInfo.scene,
            "' has been successfully saved to file at: '", fullPath, "'");
    }
}



















