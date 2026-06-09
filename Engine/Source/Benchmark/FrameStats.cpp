
#include "FrameStats.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <functional>

namespace RR
{
    FrameStats ComputeStats(const std::vector<FrameSample>& _samples)
    {
        FrameStats stats;
        if (_samples.empty()) return stats;

        stats.frameCount = _samples.size();

        std::vector<float> frameTimes;
        frameTimes.reserve(_samples.size());
        double cpuSum = 0.0f;
        double gpuSum = 0.0f;

        for (const auto& sample : _samples)
        {
            frameTimes.push_back(sample.frameTimeMs);
            cpuSum += sample.cpuMs;
            gpuSum += sample.gpuMs;
        }
        const double framesNo = static_cast<double>(frameTimes.size());

        // Averages
        const double summ = std::accumulate(frameTimes.begin(), frameTimes.end(), 0.0);
        stats.avgFrameTimeMs = static_cast<float>(summ / framesNo);
        stats.avgFps = (stats.avgFrameTimeMs > 0.0f) ? 1000.0f / stats.avgFrameTimeMs : 0.0f;
        stats.avgCpuMs = static_cast<float>(cpuSum / framesNo);
        stats.avgGpuMs = static_cast<float>(gpuSum / framesNo);

        // Min Max frametime
        const auto [min, max] = std::minmax_element(frameTimes.begin(), frameTimes.end());
        stats.minFrameTimeMs = *min;
        stats.maxFrameTimeMs = *max;

        // Populate standard deviation
        double var = 0.0f;
        for (float val : frameTimes)
        {
            const double deviation = val - stats.avgFrameTimeMs;
            var += deviation * deviation;
        }
        stats.stdDeviationMs = static_cast<float>(std::sqrt(var / framesNo));

        // Stutter count
        // if frametime > frametime * stutterThreshold we consider a spike
        std::vector<float> ascending = frameTimes;
        std::sort(ascending.begin(), ascending.end());

        const float median = ascending[ascending.size() / 2];
        const float threshold = stutterThreshold * median;

        stats.stutterCount = static_cast<int>(std::ranges::count_if(frameTimes,
            [threshold](float _val) {
                return _val >= threshold;
        }));

        // Lows
        // worst frames
        std::vector<float> descending = frameTimes;
        std::sort(descending.begin(), descending.end(), std::greater<float>());

        stats.low10Ms = MeanOfWorst(descending, 10.f);
        stats.low5Ms  = MeanOfWorst(descending, 5.0f);
        stats.low1Ms  = MeanOfWorst(descending, 1.0f);
        stats.low01Ms = MeanOfWorst(descending, 0.1f);

        return stats;
    }

    float MeanOfWorst(const std::vector<float> &_desc, float _percent)
    {
        if (_desc.empty()) return 0.0f;

        sizeT count = static_cast<sizeT>(std::ceil(_desc.size() * (_percent / 100.0f)));
        count = std::clamp<sizeT>(count, 1, _desc.size()); // min 1 frame

        double sum = 0.0f;
        for (sizeT i = 0; i < count; i++)
        {
            sum += _desc[i];
        }

        return static_cast<float>(sum / static_cast<double>(count));
    }
}
