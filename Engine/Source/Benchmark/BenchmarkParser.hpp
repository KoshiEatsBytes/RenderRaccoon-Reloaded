
#pragma once
#include <charconv>
#include <string_view>

#include "Benchmark/BenchmarkData.h"
#include "Helpers/Printer.hpp"

namespace RR
{
    class BenchmarkParser
    {
        static std::string_view Trim(std::string_view _string)
        {
            const auto begin = _string.find_first_not_of(" \t\r\n");
            if (begin == std::string_view::npos) return {};

            const auto end = _string.find_last_not_of(" \t\r\n");
            return _string.substr(begin, end - begin + 1);
        }

        // fast parser of floats
        static bool CharsToFloat(std::string_view _string, float& _out)
        {
            _string = Trim(_string);
            auto [ptr, error] = std::from_chars(
                _string.data(),
                _string.data() + _string.size(),
                _out);

            if (error == std::errc{})
            {
                return true;
            }
            return false;
        }

    public:
        static BenchmarkRun ParseBenchmarkCsv(const std::string& _text)
        {
            BenchmarkRun runData;
            const std::string_view text = _text;

            sizeT pos = 0;
            while (pos < text.size())
            {
                sizeT endOfLine = text.find('\n', pos);
                if (endOfLine == std::string_view::npos)
                {
                    endOfLine = text.size();
                }

                // Trim also drops a trailing '\r'
                const std::string_view line = Trim(text.substr(pos, endOfLine - pos));   
                pos = endOfLine + 1;

                if (line.empty()) continue;

                // Unload metadata from CSV - this includes the run detail and success
                if (line.front() == '#')
                {
                    const auto equal = line.find('=');
                    if (equal == std::string_view::npos) continue;

                    const std::string_view key = Trim(line.substr(1, equal - 1));
                    const std::string_view val = Trim(line.substr(equal + 1));

                    if      (key == "name")          runData.info.name          = std::string(val);
                    else if (key == "scene")         runData.info.scene         = std::string(val);
                    else if (key == "scenario")      runData.info.scene         = std::string(val);   
                    else if (key == "config")        runData.info.config        = std::string(val);
                    else if (key == "completed")     runData.info.completed     = (val == "1" || val == "true");
                    else if (key == "lod")           runData.info.lod           = (val == "1");
                    else if (key == "async")         runData.info.async         = (val == "1");
                    else if (key == "scheduling")    runData.info.scheduling    = (val == "1");
                    else if (key == "lodCache")      runData.info.lodCache      = (val == "1");
                    else if (key == "greedy")        runData.info.greedy        = (val == "1");
                    else if (key == "deterministic") runData.info.deterministic = (val == "1" || val == "true");
                    else if (key == "gpu")           runData.info.gpuName       = std::string(val);
                    else if (key == "cpu")           runData.info.cpuName       = std::string(val);
                    else if (key == "cores")
                    {
                        unsigned long cores = 0;
                        std::from_chars(val.data(), val.data() + val.size(), cores);
                        runData.info.coreCount = static_cast<unsigned int>(cores);
                    }
                    else if (key == "seed")
                    {
                        std::from_chars(val.data(), val.data() + val.size(), runData.info.seed);
                    }
                    else if (key == "frames")
                    {
                        unsigned long frameNum = 0;
                        std::from_chars(val.data(), val.data() + val.size(), frameNum);
                        runData.samples.reserve(frameNum);
                    }
                    continue;
                }

                // skip column header
                if (line.rfind("frameIdx", 0) == 0) continue;

                // Data row: frameIndex frameTimeMs cpuMs gpuMs
                const std::size_t c0 = line.find(',');
                const std::size_t c1 = (c0 == std::string_view::npos) ? c0 : line.find(',', c0 + 1);
                const std::size_t c2 = (c1 == std::string_view::npos) ? c1 : line.find(',', c1 + 1);

                // malformed line, skip
                if (c2 == std::string_view::npos) continue;

                FrameSample sample;
                const bool ok = CharsToFloat(line.substr(c0 + 1, c1 - c0 - 1), sample.frameTimeMs)
                             && CharsToFloat(line.substr(c1 + 1, c2 - c1 - 1), sample.cpuMs)
                             && CharsToFloat(line.substr(c2 + 1),                sample.gpuMs);
                // if sample is valid, save
                if (ok) runData.samples.push_back(sample);
            }

            // return populated run
            runData.stats = ComputeStats(runData.samples);
            return runData;
        }
    };
}
