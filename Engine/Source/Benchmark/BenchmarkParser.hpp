
#pragma once
#include <charconv>
#include <string_view>
#include <functional>
#include <sstream>
#include <algorithm>
#include <vector>

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
        // candidate run for summary
        struct SummaryInput
        {
            std::string orderKey;
            RunInfo     info;
        };

        // outcome of 3 summed runs
        struct SummaryOutcome
        {
            int written = 0;
            int skipped = 0;   // groups with no readable runs
            int partial = 0;   // not full runs with failed elemnts
            std::string csvText;
        };

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
                    else if (key == "physicalCores")
                    {
                        unsigned long physical = 0;
                        std::from_chars(val.data(), val.data() + val.size(), physical);
                        runData.info.physicalCores = static_cast<unsigned int>(physical);
                    }
                    else if (key == "workerThreads")
                    {
                        std::from_chars(val.data(), val.data() + val.size(),
                            runData.info.workerThreads);
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

        // Groups completed runs by config identity, takes the last 3 and summarizes them
        // they cant be viewed by graphs but can be usefull for data
        static SummaryOutcome BuildSummaryCsv(std::vector<SummaryInput> _runs,
            const std::function<std::string(const std::string&)>& _loadText)
        {
            SummaryOutcome outcome;

            // group by config identity
            struct Group
            {
                std::string key;
                std::vector<const SummaryInput*> entries;
            };
            std::vector<Group> groups;

            for (const SummaryInput& run : _runs)
            {
                // not completed, skip
                if (!run.info.completed) continue;

                const std::string key = run.info.name + "|" + run.info.cpuName + "|" +
                                        run.info.gpuName + "|" + std::to_string(run.info.seed);

                // match group
                auto it = std::ranges::find_if(groups,
                    [&](const Group& _group)
                    {
                        return _group.key == key;
                    });

                if (it == groups.end())
                {
                    groups.push_back({ key, {}});
                    it = groups.end() - 1;
                }
                it->entries.push_back(&run);
            }

            // median over however many runs a config managed
            const auto medianOf = [](std::vector<float> _values)
            {
                std::ranges::sort(_values);
                const sizeT n = _values.size();

                if (n % 2)
                {
                    return _values[n / 2];
                }

                return 0.5f * (_values[n / 2 - 1] + _values[n / 2]);
            };

            // spread, 0 for single run
            const auto spreadOf = [](const std::vector<float>& _values)
            {
                const auto [min, max] =
                    std::ranges::minmax_element(_values);
                return *max - *min;
            };

            // prepare for file stream
            std::ostringstream contents;

            for (auto&[key, entries] : groups)
            {
                // latest first
                std::ranges::sort(entries,
                    [](const SummaryInput* _a, const SummaryInput* _b)
                    {
                        return _a->orderKey > _b->orderKey;
                    });

                // collect up to the 3 latest parseable runs
                std::vector<BenchmarkRun> runs;
                for (const SummaryInput* entry : entries)
                {
                    if (runs.size() == 3) break;

                    BenchmarkRun run = ParseBenchmarkCsv(_loadText(entry->orderKey));
                    if (!run.samples.empty())
                    {
                        runs.push_back(std::move(run));
                    }
                }

                // nothing readable at all
                if (runs.empty())
                {
                    ++outcome.skipped;
                    continue;
                }

                if (runs.size() < 3) ++outcome.partial;

                // median and spread column pair for one field
                const auto pair = [&](auto _field)
                {
                    std::vector<float> values;
                    values.reserve(runs.size());

                    for (const BenchmarkRun& run : runs)
                    {
                        values.push_back(_field(run));
                    }
                    contents << "," << medianOf(values) << "," << spreadOf(values);
                };

                // ms lows reported as fps, converted per run BEFORE aggregating
                const auto pairFps = [&](auto _field)
                {
                    pair([&](const BenchmarkRun& _run)
                    {
                        const float ms = _field(_run);
                        return ms > 0.0f ? 1000.0f / ms : 0.0f;
                    });
                };

                const RunInfo& info = runs[0].info;

                contents << "\"" << info.name  << "\","  << info.renderDistance << ","
                                 << info.lod   << ","    << info.aggregation    << "," << info.greedy << ","
                                 << info.async << ","    << info.scheduling     << ","
                                 << info.seed  << ",\""  << info.cpuName        << "\",\"" << info.gpuName << "\","
                                 << runs.size();

                // pair runs and stream back to contents
                pair   ([](const BenchmarkRun& _run) { return _run.stats.avgFps; });
                pairFps([](const BenchmarkRun& _run) { return _run.stats.low10Pc; });
                pairFps([](const BenchmarkRun& _run) { return _run.stats.low5Pc; });
                pairFps([](const BenchmarkRun& _run) { return _run.stats.low1Pc; });
                pairFps([](const BenchmarkRun& _run) { return _run.stats.low01Pc; });
                pair   ([](const BenchmarkRun& _run) { return _run.stats.avgFrameTimeMs; });
                pair   ([](const BenchmarkRun& _run) { return _run.stats.maxFrameTimeMs; });
                pair   ([](const BenchmarkRun& _run) { return _run.stats.stdDeviationMs; });
                pair   ([](const BenchmarkRun& _run) { return static_cast<float>(_run.stats.stutterCount); });
                pair   ([](const BenchmarkRun& _run) { return _run.stats.avgCpuMs; });
                pair   ([](const BenchmarkRun& _run) { return _run.stats.avgGpuMs; });
                pair   ([](const BenchmarkRun& _run) { return _run.info.warmUpSeconds; });
                pair   ([](const BenchmarkRun& _run) { return _run.stats.coverageMin; });

                contents << "," << info.steadyDraws << "," << info.steadyTris << ","
                         << runs[0].stats.frameCount << "\n";

                ++outcome.written;
            }

            if (outcome.written == 0) return outcome;

            outcome.csvText =
                "# Artefact benchmark summary, MEDIAN of up to the 3 latest matching completed runs, sp = max-min spread, lows in fps, runs = sample count\n"
                "name,rd,lod,la,gm,mt,ab,seed,cpu,gpu,runs,"
                "avgFps,avgFps_sp,low10Fps,low10Fps_sp,low5Fps,low5Fps_sp,low1Fps,low1Fps_sp,low01Fps,low01Fps_sp,"
                "avgMs,avgMs_sp,maxMs,maxMs_sp,stdMs,stdMs_sp,stutters,stutters_sp,"
                "cpuMs,cpuMs_sp,gpuMs,gpuMs_sp,loadS,loadS_sp,covMin,covMin_sp,"
                "steadyDraws,steadyTris,frames\n"
                + contents.str();

            return outcome;
        }
    };
}
