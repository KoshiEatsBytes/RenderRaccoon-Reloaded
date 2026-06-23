
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
        if (_runInfo.lod)        _str.append("-LOD");
        if (_runInfo.async)      _str.append("-MT");
        if (_runInfo.scheduling) _str.append("-SM");
        if (_runInfo.lodCache)   _str.append("-LC");
        if (_runInfo.greedy)     _str.append("-GM");

        // and render dist
        _str.append("-R" + std::to_string(_runInfo.renderDistance));
    }
}

namespace DETERMINISTIC
{
    using uInt32 = std::uint32_t;

    constexpr uInt32 kDeterministicSeed = 2498846564;

    // Benchmark data
    constexpr int    kBaselineRenderDistance = 24;
    constexpr int    kLodRenderDistance      = 384;

    enum class SCENE : std::uint8_t
    {
        BASELINE,

        LOD_ONLY,
        MT_ONLY,
        SS_ONLY,
        LOD_LC,
        GM_ONLY,

        ALL_RD32,
        ALL_RD384,

        COUNT
    };

    // Helper to return the deterministic scene pre-set for the requested scene
    inline RR::RunInfo GetRunPreset(SCENE _scene)
    {
        RR::RunInfo runInfo;

        // true for every deterministic run
        runInfo.deterministic = true;
        runInfo.seed = kDeterministicSeed;

        switch (_scene)
        {
            case SCENE::BASELINE:
            {
                runInfo.name  = "Deterministic Baseline";
                runInfo.scene = "BASELINE-R" + std::to_string(kBaselineRenderDistance);
                // Baseline no techniques
                runInfo.lod        = false;
                runInfo.async      = false;
                runInfo.scheduling = false;
                runInfo.lodCache   = false;
                runInfo.greedy     = false;
                // Render dist
                runInfo.renderDistance = kBaselineRenderDistance;
            }
            break;

            case SCENE::LOD_ONLY:
            {
                runInfo.name  = "Deterministic LOD Only";
                runInfo.scene = "LOD-R" + std::to_string(kBaselineRenderDistance);
                // LOD only
                runInfo.lod        = true;
                runInfo.async      = false;
                runInfo.scheduling = false;
                runInfo.lodCache   = false;
                runInfo.greedy     = false;
                // Render dist
                runInfo.renderDistance = kBaselineRenderDistance;
            }
            break;

            case SCENE::MT_ONLY:
            {
                runInfo.name  = "Deterministic Multi-Threading Only";
                runInfo.scene = "MT-R" + std::to_string(kBaselineRenderDistance);
                // MT only
                runInfo.lod        = false;
                runInfo.async      = true;
                runInfo.scheduling = false;
                runInfo.lodCache   = false;
                runInfo.greedy     = false;
                // Render dist
                runInfo.renderDistance = kBaselineRenderDistance;
            }
            break;

            case SCENE::SS_ONLY:
            {
                runInfo.name  = "Deterministic Smart-Scheduling Only";
                runInfo.scene = "SM-R" + std::to_string(kBaselineRenderDistance);
                // SS only
                runInfo.lod        = false;
                runInfo.async      = false;
                runInfo.scheduling = true;
                runInfo.lodCache   = false;
                runInfo.greedy     = false;
                // Render dist
                runInfo.renderDistance = kBaselineRenderDistance;
            }
            break;

            case SCENE::LOD_LC:
            {
                runInfo.name  = "Deterministic LOD and LOD Cache";
                runInfo.scene = "LOD-LC-R" + std::to_string(kBaselineRenderDistance);
                // LOD and LC only
                runInfo.lod        = true;
                runInfo.async      = false;
                runInfo.scheduling = false;
                runInfo.lodCache   = true;
                runInfo.greedy     = false;
                // Render dist
                runInfo.renderDistance = kBaselineRenderDistance;
            }
                break;
            case SCENE::GM_ONLY:
            {
                runInfo.name  = "Deterministic Greedy Meshing Only";
                runInfo.scene = "GM-R" + std::to_string(kBaselineRenderDistance);
                // GM only
                runInfo.lod        = false;
                runInfo.async      = false;
                runInfo.scheduling = false;
                runInfo.lodCache   = false;
                runInfo.greedy     = true;
                // Render dist
                runInfo.renderDistance = kBaselineRenderDistance;
            }
            break;

            case SCENE::ALL_RD32:
            {
                runInfo.name  = "Deterministic ALL RD " + std::to_string(kBaselineRenderDistance);
                runInfo.scene = "ALL-R" + std::to_string(kBaselineRenderDistance);
                // All techniques
                runInfo.lod        = true;
                runInfo.async      = true;
                runInfo.scheduling = true;
                runInfo.lodCache   = true;
                runInfo.greedy     = true;
                // Render dist
                runInfo.renderDistance = kBaselineRenderDistance;
            }
            break;

            case SCENE::ALL_RD384:
            {
                runInfo.name  = "Deterministic ALL RD " + std::to_string(kLodRenderDistance);
                runInfo.scene = "ALL-R" + std::to_string(kLodRenderDistance);
                // All techniques + extreme RD
                runInfo.lod        = true;
                runInfo.async      = true;
                runInfo.scheduling = true;
                runInfo.lodCache   = true;
                runInfo.greedy     = true;
                // Render dist
                runInfo.renderDistance = kLodRenderDistance;
            }
            break;

            case SCENE::COUNT:
            {
                runInfo.deterministic = false;
                runInfo.name  = "INVALID SCENE REQUEST";
                runInfo.scene = "INVALID SCENE REQUEST";
                RR::Error("REQUEST INVALID RUN PRESET FOR DETERMINISTIC BENCHMARK");
            }
            break;
        }

        return runInfo;
    }
}

namespace CUSTOM
{
    enum class SCENE : std::uint8_t
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

        // Populate strings
        NAMING::AppendDetails(_rInfo, name);
        NAMING::AppendDetails(_rInfo, scene);

        _rInfo.scene = scene;
        _rInfo.name  = name;

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
                break;

        }
    }
}
