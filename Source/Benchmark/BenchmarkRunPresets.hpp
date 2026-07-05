
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
        if (_runInfo.scheduling)  _str.append("-AB");

        // and render dist
        _str.append("-R" + std::to_string(_runInfo.renderDistance));
    }
}

namespace DETERMINISTIC
{
    using uInt32 = std::uint32_t;
    using uInt8  = std::uint8_t;

    constexpr uInt32 kDeterministicSeed = 3053828723;

    // Benchmark run matrix - 16 for now trying to test everything without overwhelming of runs
    enum class SCENE : uInt8
    {
        BASELINE,
        BASE_MT_16,
        BASE_32,
        BASE_MT_32,
        BASE_MT_AB_32,
        LOD_64,
        LOD_LA_64,
        LOD_GM_64,
        LOD_MT_64,
        LOD_LA_256,
        LOD_LA_MT_256,
        LOD_LA_GM_MT_256,
        FULL_256,
        LOD_LA_MT_384,
        LOD_LA_GM_MT_384,
        FULL_384,

        COUNT
    };

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

    // benchmark matrix, as the counter goes up next one is selected
    constexpr RowSpec kMatrix[] = {
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

    static_assert(std::size(kMatrix) == static_cast<std::size_t>(SCENE::COUNT),
        "Benchmark matrix table out of sync with scene enum");

    //  Deterministic sequence
    constexpr uInt8 kSceneCount            = static_cast<uInt8>(SCENE::COUNT);
    inline    uInt8 gCurrentSceneStep      = static_cast<uInt8>(SCENE::BASELINE);
    inline    uInt8 gDeterministicFailures = 0;

    // Helper to return the deterministic scene per set for the requested scene
    inline RR::RunInfo GetRunPreset(SCENE _scene)
    {
        RR::RunInfo runInfo;

        // true for every deterministic run
        runInfo.deterministic = true;
        runInfo.seed = kDeterministicSeed;

        // invalid run requested if somehow you mamaged??
        if (_scene >= SCENE::COUNT)
        {
            runInfo.deterministic = false;
            runInfo.name  = "INVALID SCENE REQUEST";
            runInfo.scene = "INVALID SCENE REQUEST";
            RR::Error("REQUEST INVALID RUN PRESET FOR DETERMINISTIC BENCHMARK");
            return runInfo;
        }

        // table tied
        const RowSpec& runRow = kMatrix[static_cast<uInt8>(_scene)];
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
