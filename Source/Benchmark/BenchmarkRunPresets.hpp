
#pragma once
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
        if (_runInfo.scheduling)  _str.append("-SM");

        // and render dist
        _str.append("-R" + std::to_string(_runInfo.renderDistance));
    }
}

namespace DETERMINISTIC
{
    using uInt32 = std::uint32_t;
    using uInt8  = std::uint8_t;

    constexpr uInt32 kDeterministicSeed = 3053828723;

    // Benchmark data
    constexpr int    kBaselineRenderDistance = 16;
    constexpr int    kLodRenderDistance      = 24; // Bump up once LOD sets in

    enum class SCENE : uInt8
    {
        BASELINE,

        // Rendering
        LOD_ONLY,
        LOD_AGG,
        LOD_GM,

        // Scheduling
        MT_ONLY,
        SS_ONLY,

        ALL_BASE,
        ALL_HORIZON,

        COUNT
    };

    //  Deterministic sequence
    constexpr uInt8 kSceneCount            = static_cast<uInt8>(SCENE::COUNT);
    inline    uInt8 gCurrentSceneStep      = static_cast<uInt8>(SCENE::BASELINE);
    inline    uInt8 gDeterministicFailures = 0;

    // Helper to return the deterministic scene pre-set for the requested scene
    inline RR::RunInfo GetRunPreset(SCENE _scene)
    {
        RR::RunInfo runInfo;
        std::string name  = "DETERMINISTIC";
        std::string scene = "D";

        // true for every deterministic run
        runInfo.deterministic = true;
        runInfo.seed = kDeterministicSeed;

        switch (_scene)
        {
            case SCENE::BASELINE:
            {
                // Baseline no techniques
                runInfo.lod         = false;
                runInfo.async       = false;
                runInfo.scheduling  = false;
                runInfo.aggregation = false;
                runInfo.greedy      = false;
                // Render dist
                runInfo.renderDistance = kBaselineRenderDistance;

                // name and scene
                name.append ("-BASELINE-R" + std::to_string(runInfo.renderDistance));
                scene.append("-BASE-R" + std::to_string(runInfo.renderDistance));
            }
            break;

            case SCENE::LOD_ONLY:
            {
                // LOD only
                runInfo.lod         = true;
                runInfo.async       = false;
                runInfo.scheduling  = false;
                runInfo.aggregation = false;
                runInfo.greedy      = false;
                // Render dist
                runInfo.renderDistance = kBaselineRenderDistance;

                // name and scene
                NAMING::AppendDetails(runInfo, name);
                NAMING::AppendDetails(runInfo, scene);
            }
            break;

            case SCENE::MT_ONLY:
            {
                // MT only
                runInfo.lod         = false;
                runInfo.async       = true;
                runInfo.scheduling  = false;
                runInfo.aggregation = false;
                runInfo.greedy      = false;
                // Render dist
                runInfo.renderDistance = kBaselineRenderDistance;

                // name and scene
                NAMING::AppendDetails(runInfo, name);
                NAMING::AppendDetails(runInfo, scene);
            }
            break;

            case SCENE::SS_ONLY:
            {
                // SS only
                runInfo.lod         = false;
                runInfo.async       = false;
                runInfo.scheduling  = true;
                runInfo.aggregation = false;
                runInfo.greedy      = false;
                // Render dist
                runInfo.renderDistance = kBaselineRenderDistance;

                // name and scene
                NAMING::AppendDetails(runInfo, name);
                NAMING::AppendDetails(runInfo, scene);
            }
            break;

            case SCENE::LOD_AGG:
            {
                // LOD + aggregation
                runInfo.lod         = true;
                runInfo.async       = false;
                runInfo.scheduling  = false;
                runInfo.aggregation = true;
                runInfo.greedy      = false;
                // Render dist
                runInfo.renderDistance = kBaselineRenderDistance;

                // name and scene
                NAMING::AppendDetails(runInfo, name);
                NAMING::AppendDetails(runInfo, scene);
            }
                break;
            case SCENE::LOD_GM:
            {
                // LOD + greedy
                runInfo.lod         = true;
                runInfo.async       = false;
                runInfo.scheduling  = false;
                runInfo.aggregation = false;
                runInfo.greedy      = true;
                // Render dist
                runInfo.renderDistance = kBaselineRenderDistance;

                // name and scene
                NAMING::AppendDetails(runInfo, name);
                NAMING::AppendDetails(runInfo, scene);
            }
            break;

            case SCENE::ALL_BASE:
            {
                // All techniques
                runInfo.lod         = true;
                runInfo.async       = true;
                runInfo.scheduling  = true;
                runInfo.aggregation = true;
                runInfo.greedy      = true;
                // Render dist
                runInfo.renderDistance = kBaselineRenderDistance;

                // name and scene
                name.append ("-ALL-R" + std::to_string(runInfo.renderDistance));
                scene.append("-ALL-R" + std::to_string(runInfo.renderDistance));
            }
            break;

            case SCENE::ALL_HORIZON:
            {
                // All techniques + extreme RD
                runInfo.lod         = true;
                runInfo.async       = true;
                runInfo.scheduling  = true;
                runInfo.aggregation = true;
                runInfo.greedy      = true;
                // Render dist
                runInfo.renderDistance = kLodRenderDistance;

                // name and scene
                name.append ("-ALL-R" + std::to_string(runInfo.renderDistance));
                scene.append("-ALL-R" + std::to_string(runInfo.renderDistance));
            }
            break;

            case SCENE::COUNT:
            {
                runInfo.deterministic = false;
                name  = "INVALID SCENE REQUEST";
                scene = "INVALID SCENE REQUEST";
                RR::Error("REQUEST INVALID RUN PRESET FOR DETERMINISTIC BENCHMARK");
            }
            break;
        }

        runInfo.name  = name;
        runInfo.scene = scene;
        return runInfo;
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
