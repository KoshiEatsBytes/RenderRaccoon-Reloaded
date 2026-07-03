
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
        // _headerOnly = stop at the first data row. Cheap metadata-only parse for building run
        static BenchmarkRun ParseBenchmarkCsv(const std::string& _text, bool _headerOnly = false)
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
                    else if (key == "aggregation")   runData.info.aggregation   = (val == "1");
                    else if (key == "greedy")        runData.info.greedy        = (val == "1");
                    else if (key == "async")         runData.info.async         = (val == "1");
                    else if (key == "scheduling")    runData.info.scheduling    = (val == "1");
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
                    else if (key == "renderDistance")
                    {
                        std::from_chars(val.data(), val.data() + val.size(), runData.info.renderDistance);
                    }
                    else if (key == "warmUpSeconds")
                    {
                        CharsToFloat(val, runData.info.warmUpSeconds);
                    }
                    else if (key == "steadyDraws")
                    {
                        std::from_chars(val.data(), val.data() + val.size(), runData.info.steadyDraws);
                    }
                    else if (key == "steadyTris")
                    {
                        std::from_chars(val.data(), val.data() + val.size(), runData.info.steadyTris);
                    }
                    else if (key == "frames")
                    {
                        unsigned long frameNum = 0;
                        std::from_chars(val.data(), val.data() + val.size(), frameNum);
                        if (!_headerOnly) runData.samples.reserve(frameNum);
                    }
                    continue;
                }

                // metadata block is over
                if (_headerOnly) break;

                // skip column header
                if (line.rfind("frameIdx", 0) == 0) continue;

                // Data row: frameIdx,frameTimeMs,cpuMs,gpuMs,simTime,posX,posY,posZ,coverage
                std::size_t comma[8];

                int found = 0;
                for (std::size_t i = 0; i < line.size() && found < 8; ++i)
                {
                    if (line[i] == ',') comma[found++] = i;
                }

                // Discard if not expected comma vals e.g. malformed line
                if (found < 8) continue;

                FrameSample sample;
                const bool ok =
                       CharsToFloat(line.substr(comma[0]+1, comma[1]-comma[0]-1), sample.frameTimeMs)
                    && CharsToFloat(line.substr(comma[1]+1, comma[2]-comma[1]-1), sample.cpuMs)
                    && CharsToFloat(line.substr(comma[2]+1, comma[3]-comma[2]-1), sample.gpuMs)
                    && CharsToFloat(line.substr(comma[3]+1, comma[4]-comma[3]-1), sample.simTime)
                    && CharsToFloat(line.substr(comma[4]+1, comma[5]-comma[4]-1), sample.posX)
                    && CharsToFloat(line.substr(comma[5]+1, comma[6]-comma[5]-1), sample.posY)
                    && CharsToFloat(line.substr(comma[6]+1, comma[7]-comma[6]-1), sample.posZ)
                    && CharsToFloat(line.substr(comma[7]+1),                        sample.coverage);

                // if valid save to vec
                if (ok) runData.samples.push_back(sample);
            }

            // return populated run
            runData.stats = ComputeStats(runData.samples);
            return runData;
        }
    };
}
