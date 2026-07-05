
#pragma once
#include <vector>
#include <nlohmann/json.hpp>

#include "Engine.h"
#include "Benchmark/BenchmarkData.h"
#include "Helpers/Printer.hpp"
#include "WorldGen/Noise.hpp"
#include "WorldGen/WorldGenConfig.h"

namespace NAMING
{
    inline void AppendDetails(const RR::RunInfo& _runInfo, std::string& _str)
    {
        // Appends to str techniques used
        if (_runInfo.lod)         _str.append("-LOD");
        if (_runInfo.aggregation) _str.append("-LA");
        if (_runInfo.greedy)      _str.append("-GM");
        if (_runInfo.async)       _str.append("-MT");
        if (_runInfo.scheduling)  _str.append("-AB");

        // and render dist
        _str.append("-R" + std::to_string(_runInfo.renderDistance));
    }
}

namespace CONFIG
{
    // Load text from config folder
    inline std::string LoadText(const std::string& _relPath, const char* _fallback)
    {
        std::string text = RR::Engine::GetInstance().GetFileSystem()
            .LoadAssetFileText(_relPath);

        if (text.empty())
        {
            RR::Warn("[CONFIG] '", _relPath, "' is missing! using compiled fallback");
            text = _fallback;
        }
        return text;
    }
}

namespace DETERMINISTIC
{
    using uInt32 = std::uint32_t;
    using uInt8  = std::uint8_t;

    constexpr uInt32 kDeterministicSeed = 3053828723;

    // Matrix row, easier readable
    struct RowSpec
    {
        bool lod;
        bool la;
        bool gm;
        bool mt;
        bool ab;
        int  rd;
    };

    // benchmark matrix fallback, if the config file explodes and doesnt load
    constexpr RowSpec kFallbackMatrix[] = {
        { false, false, false, false, false,  16 }, // BASELINE
        { false, false, false, true,  false,  16 }, // BASE MT 16
        { false, false, false, false, false,  32 }, // BASE 32
        { false, false, false, true,  false,  32 }, // BASE MT 32
        { false, false, false, true,  true,   32 }, // BASE MT AB 32
        { true,  false, false, false, false,  64 }, // LOD 64
        { true,  true,  false, false, false,  64 }, // LOD LA 64
        { true,  false, true,  false, false,  64 }, // LOD GM 64
        { true,  false, false, true,  false,  64 }, // LOD MT 64
        { true,  true,  false, false, false, 256 }, // LOD LA 256
        { true,  true,  false, true,  false, 256 }, // LOD LA MT 256
        { true,  true,  true,  true,  false, 256 }, // LOD LA GM MT 256
        { true,  true,  true,  true,  true,  256 }, // ALL 256
        { true,  true,  false, true,  false, 384 }, // LOD LA MT 384
        { true,  true,  true,  true,  false, 384 }, // LOD LA GM MT 384
        { true,  true,  true,  true,  true,  384 }, // ALL 384
    };

    //  Deterministic sequence
    inline uInt8 gCurrentSceneStep      = 0;
    inline uInt8 gDeterministicFailures = 0;

    // Benchmark run matrix, loaded from config json file
    inline std::vector<RowSpec> gMatrix;
    inline bool                 gMatrixLoaded = false;

    inline const std::vector<RowSpec>& GetMatrix()
    {
        if (gMatrixLoaded) return gMatrix;
        gMatrixLoaded = true;

        const std::string text = RR::Engine::GetInstance().GetFileSystem()
            .LoadAssetFileText("Config/DeterministicMatrix.json");

        if (!text.empty())
        {
            // try parsing json, if fail use fallback defualt
            try
            {
                const nlohmann::json json = nlohmann::json::parse(text);

                for (const auto& entry : json.at("rows"))
                {
                    RowSpec row;
                    row.lod = entry.value("lod", false);
                    row.la  = entry.value("la",  false);
                    row.gm  = entry.value("gm",  false);
                    row.mt  = entry.value("mt",  false);
                    row.ab  = entry.value("ab",  false);
                    row.rd  = entry.value("rd",  16);

                    // Dependencies, warn if invalid loaded
                    if ((row.la || row.gm) && !row.lod)
                    {
                        RR::Error("[DETERMINISTIC BENCH CONFIG] row skipped! LA/GM require LOD");
                        continue;
                    }
                    if (row.ab && !row.mt)
                    {
                        RR::Error("[DETERMINISTIC BENCH CONFIG] row skipped! AB requires MT");
                        continue;
                    }
                    if (row.rd < 2)
                    {
                        RR::Error("[DETERMINISTIC BENCH CONFIG] row skipped! rd must be < 2");
                        continue;
                    }

                    gMatrix.push_back(row);
                }
            }
            catch (const nlohmann::json::exception& error)
            {
                RR::Error("[DETERMINISTIC BENCH CONFIG] Config parse failed '", error.what(), "' using fallback");
                gMatrix.clear();
            }
        }

        // Empty, corrupted or bigger than uInt8 max
        if (gMatrix.empty() || gMatrix.size() > 255)
        {
            gMatrix.assign(std::begin(kFallbackMatrix), std::end(kFallbackMatrix));
            RR::Warn("[DETERMINISTIC BENCH CONFIG] using compiled fallback bench settings (", gMatrix.size(), " rows)");
        }
        else
        {
            RR::Success("[DETERMINISTIC BENCH CONFIG] loaded ", gMatrix.size(), " rows from Config");
        }

        return gMatrix;
    }

    inline uInt8 GetSceneCount()
    {
        return static_cast<uInt8>(GetMatrix().size());
    }

    // Helper to return the deterministic scene pre set for the requested step
    inline RR::RunInfo GetRunPreset(uInt8 _step)
    {
        RR::RunInfo runInfo;

        // true for every deterministic run
        runInfo.deterministic = true;
        runInfo.seed = kDeterministicSeed;

        // invalid run requested if somehow you mamaged??
        if (_step >= GetSceneCount())
        {
            runInfo.deterministic = false;
            runInfo.name  = "INVALID SCENE REQUEST";
            runInfo.scene = "INVALID SCENE REQUEST";
            RR::Error("REQUEST INVALID RUN PRESET FOR DETERMINISTIC BENCHMARK");
            return runInfo;
        }

        // table tied
        const RowSpec& runRow = GetMatrix()[_step];
        runInfo.lod            = runRow.lod;
        runInfo.aggregation    = runRow.la;
        runInfo.greedy         = runRow.gm;
        runInfo.async          = runRow.mt;
        runInfo.scheduling     = runRow.ab;
        runInfo.renderDistance = runRow.rd;

        std::string name  = "DETERMINISTIC";
        std::string scene = "D";

        // baseline specific have to be named speratale coz they have no technique enabled
        if (!runRow.lod && !runRow.la && !runRow.gm && !runRow.mt && !runRow.ab)
        {
            name.append ("-BASELINE-R" + std::to_string(runRow.rd));
            scene.append("-BASE-R"     + std::to_string(runRow.rd));
        }
        else
        {
            NAMING::AppendDetails(runInfo, name);
            NAMING::AppendDetails(runInfo, scene);
        }

        runInfo.name  = name;
        runInfo.scene = scene;
        return runInfo;
    }

    // Per row label for deterministic bench description
    inline std::string RowLabel(const RowSpec& _row)
    {
        std::string label;
        if (_row.lod) label += "LOD";
        if (_row.la)  label += label.empty() ? "LA" : " + LA";
        if (_row.gm)  label += label.empty() ? "GM" : " + GM";
        if (_row.mt)  label += label.empty() ? "MT" : " + MT";
        if (_row.ab)  label += label.empty() ? "AB" : " + AB";

        if (label.empty())
        {
            return "RD" + std::to_string(_row.rd);
        }

        return label + " at RD" + std::to_string(_row.rd);
    }

    // Menu description, load from .exe file
    inline std::string GetDeterministicDescription()
    {
        static std::string cached;
        if (!cached.empty()) return cached;

        std::string prose = RR::Engine::GetInstance().GetFileSystem()
            .LoadAssetFileText("Config/DeterministicDescription.txt");

        // if nothing loads use fallback so its not empty
        if (prose.empty())
        {
            RR::Warn("[DETERMINISTIC BENCH CONFIG] description file missing! Using compiled fallback");

            prose = "Runs the full deterministic sequence, same on every machine, with a fixed seed, "
                    "scripted camera path, and locked technique combinations from the baseline up to "
                    "the full stack.\n\n"
                    "On some weaker hardware some runs might fail to load or be aborted mid-run if "
                    "a minimum Frame-Rate is not kept, nothing will be logged for that run.\n\n"
                    "Each run fully loads the environment first, during this process no logging "
                    "except load time will happen, and once the loading has completed the run "
                    "will execute and log as expected.\n\n"
                    "On a successfully finished run the result will be logged to disk under the "
                    "'Benchmarks' folder on the same location as the executable.\n\n"
                    "The full sequence takes roughly 20-30 minutes on mid hardware, for cleaner data: "
                    "plug in the charger, close background apps, and leave the machine alone until it "
                    "finishes, make sure the machine doesn't go into sleep.";
        }

        // make legend so user knows whats each
        prose += "\n\nLEGEND: "
                 "\n  RD: Render Distance"
                 "\n LOD: Level Of Detail"
                 "\n  LA: LOD Aggregation"
                 "\n  GM: Greedy Meshing"
                 "\n  MT: Multi Threading"
                 "\n  AB: Adaptive Budgeting"
                 "\n\nSEQUENCE:";

        const auto& matrix = GetMatrix();
        for (std::size_t i = 0; i < matrix.size(); ++i)
        {
            prose += "\n " + std::to_string(i + 1) + " - " + RowLabel(matrix[i]);
        }

        cached = prose;
        return cached;
    }
}

namespace CUSTOM
{
    using uInt8 = std::uint8_t;

    enum class SCENE : uInt8
    {
        BENCHMARK_STANDARD,

        PLAINS,
        FOREST,
        DESERT,
        TAIGA,
        TUNDRA,
        SAVANNA,

        COUNT
    };

    // Map enum entries to name
    static constexpr const char* kSceneNames[] = {
        "Standard (same as deterministic)",
        "Plains",
        "Forest",
        "Desert",
        "Taiga",
        "Tundra",
        "Savanna"
    };
    static_assert(std::size(kSceneNames) == static_cast<int>(SCENE::COUNT), "Custom scene names out of sync with enum");

    // Configures the custom benchmark with the details given
    inline void GetRunPreset(SCENE _scene, RR::RunInfo& _rInfo, WORLDGEN::WorldGenConfig& _genCfg)
    {
        std::string name  = "Custom";
        std::string scene = "C";

        // Custom is NOT deterministic, but use same seed
        _rInfo.deterministic = false;
        _rInfo.seed = DETERMINISTIC::kDeterministicSeed;

        // Customize World Gen Per Pre-set
        switch (_scene)
        {
            case SCENE::BENCHMARK_STANDARD:
                // Leave world gen as-is
                break;

            case SCENE::PLAINS:
                // Only generate plains
                _genCfg.tempCold          = 0.0f;
                _genCfg.tempHot           = 1.0f;
                _genCfg.mountainChance    = 0.0f;
                _genCfg.tundraHumidThresh = 0.5f;
                _genCfg.plainsHumidThresh = 1.0f;
                _genCfg.desertHumidThresh = 0.5f;
                _genCfg.mesaRarity        = 1.0f;

                name.append("-Plains");
                scene.append("-PL");
                break;

            case SCENE::FOREST:
                // Only generate forest
                _genCfg.tempCold          = 0.0f;
                _genCfg.tempHot           = 1.0f;
                _genCfg.mountainChance    = 0.0f;
                _genCfg.tundraHumidThresh = 0.5f;
                _genCfg.plainsHumidThresh = 0.0f;
                _genCfg.desertHumidThresh = 0.5f;
                _genCfg.mesaRarity        = 1.0f;

                name.append("-Forest");
                scene.append("-FR");
                break;

            case SCENE::DESERT:
                // Only generate desert
                _genCfg.tempCold          = 0.0f;
                _genCfg.tempHot           = 0.001f;
                _genCfg.mountainChance    = 0.0f;
                _genCfg.tundraHumidThresh = 0.5f;
                _genCfg.plainsHumidThresh = 0.5f;
                _genCfg.desertHumidThresh = 1.0f;
                _genCfg.mesaRarity        = 1.0f;

                name.append("-Desert");
                scene.append("-DS");
                break;

            case SCENE::SAVANNA:
                // Only generate savanna
                _genCfg.tempCold          = 0.0f;
                _genCfg.tempHot           = 0.001f;
                _genCfg.mountainChance    = 0.0f;
                _genCfg.tundraHumidThresh = 0.5f;
                _genCfg.plainsHumidThresh = 0.5f;
                _genCfg.desertHumidThresh = 0.0f;
                _genCfg.mesaRarity        = 1.0f;

                name.append("-Savanna");
                scene.append("-SV");
                break;

            case SCENE::TAIGA:
                // Only generate taiga
                _genCfg.tempCold          = 1.0f;
                _genCfg.tempHot           = 1.0f;
                _genCfg.mountainChance    = 0.0f;
                _genCfg.tundraHumidThresh = 0.0f;
                _genCfg.plainsHumidThresh = 0.5f;
                _genCfg.desertHumidThresh = 0.5f;
                _genCfg.mesaRarity        = 1.0f;

                name.append("-Taiga");
                scene.append("-TG");
                break;

            case SCENE::TUNDRA:
                // Only generate tundra
                _genCfg.tempCold          = 1.0f;
                _genCfg.tempHot           = 1.0f;
                _genCfg.mountainChance    = 0.0f;
                _genCfg.tundraHumidThresh = 1.0f;
                _genCfg.plainsHumidThresh = 0.5f;
                _genCfg.desertHumidThresh = 0.5f;
                _genCfg.mesaRarity        = 1.0f;

                name.append("-Tundra");
                scene.append("-TD");
                break;

        }

        // Populate strings
        NAMING::AppendDetails(_rInfo, name);
        NAMING::AppendDetails(_rInfo, scene);

        _rInfo.scene = scene;
        _rInfo.name  = name;
    }
}
