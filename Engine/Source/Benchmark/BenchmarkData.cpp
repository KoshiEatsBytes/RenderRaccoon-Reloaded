
#include "BenchmarkData.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <functional>

namespace
{
    using sizeT = std::size_t;

    float MeanOfWorst(const std::vector<float> &_desc, float _percent)
    {
        if (_desc.empty()) return 0.0f;

        sizeT count = static_cast<sizeT>(std::ceil(_desc.size() * (_percent / 100.0f)));
        count = std::clamp<sizeT>(count, 1, _desc.size()); // min 1 frame

        double sum = 0.0;
        for (sizeT i = 0; i < count; i++)
        {
            sum += _desc[i];
        }

        return static_cast<float>(sum / static_cast<double>(count));
    }
}

namespace RR
{
    FrameStats ComputeStats(const std::vector<FrameSample>& _samples)
    {
        FrameStats stats;
        if (_samples.empty()) return stats;

        stats.frameCount = _samples.size();

        std::vector<float> frameTimes;
        std::vector<float> coverages;
        frameTimes.reserve(_samples.size());
        coverages.reserve (_samples.size());
        double cpuSum  = 0.0;
        double gpuSum  = 0.0;
        double covSum  = 0.0;
        sizeT gpuValid = 0;

        for (const auto& sample : _samples)
        {
            frameTimes.push_back(sample.frameTimeMs);
            cpuSum += sample.cpuMs;

            coverages.push_back(sample.coverage);
            covSum += sample.coverage;

            // document
            if (sample.gpuMs > 0.0f)
            {
                gpuSum += sample.gpuMs;
                gpuValid++;
            }
        }
        const double framesNo = static_cast<double>(frameTimes.size());

        // Averages
        const double summ = std::accumulate(frameTimes.begin(), frameTimes.end(), 0.0);
        stats.avgFrameTimeMs = static_cast<float>(summ / framesNo);
        stats.avgFps = (stats.avgFrameTimeMs > 0.0f) ? 1000.0f / stats.avgFrameTimeMs : 0.0f;
        stats.avgCpuMs = static_cast<float>(cpuSum / framesNo);
        stats.avgGpuMs = gpuValid ? static_cast<float>(gpuSum / gpuValid) : 0.0f;
        stats.gpuSampleCount = gpuValid;

        // Min Max frametime
        const auto [min, max] = std::minmax_element(frameTimes.begin(), frameTimes.end());
        stats.minFrameTimeMs = *min;
        stats.maxFrameTimeMs = *max;

        // Coverage
        stats.coverageAvg = static_cast<float>(covSum / framesNo);
        std::ranges::sort(coverages);
        stats.coverageMin    = coverages.front();
        stats.coverageLow1Pc = MeanOfWorst(coverages, 1.0f);

        // Populate standard deviation
        double var = 0.0;
        for (float val : frameTimes)
        {
            const double deviation = val - stats.avgFrameTimeMs;
            var += deviation * deviation;
        }
        stats.stdDeviationMs = static_cast<float>(std::sqrt(var / framesNo));

        // Lows
        // worst frames
        std::vector<float> descending = frameTimes;
        std::sort(descending.begin(), descending.end(), std::greater<float>());

        stats.low10Pc = MeanOfWorst(descending, 10.f);
        stats.low5Pc  = MeanOfWorst(descending, 5.0f);
        stats.low1Pc  = MeanOfWorst(descending, 1.0f);
        stats.low01Pc = MeanOfWorst(descending, 0.1f);

        // Stutter
        // if the different between two frames is higher than the stutter delta, log
        unsigned int stutters = 0;
        for (sizeT i = 1; i < frameTimes.size(); i++)
        {
            if (frameTimes[i] - frameTimes[i - 1] > kStutterDeltaMs)
            {
                stutters++;
            }
        }
        stats.stutterCount = stutters;

        return stats;
    }
}
