
#pragma once
#include <cstdint>

#include "BiomeID.h"

namespace WORLDGEN
{
    struct WorldGenConfig
    {
        using uInt32 = std::uint32_t;

        uInt32 seed = 456356;

        // BASE TERRAIN ------------------------------------------------------------------------------------------------
        float heightScale   = 128.f; // noise in block spacing
        int   heightOctaves = 5;
        int   dirtDepth     = 4;     // Dirt thickness under grass

        // BIOMES ------------------------------------------------------------------------------------------------------
        int biomeBaseHeight[(int)BIOME::COUNT] = {
            66,  // PLAINS
            71,  // FOREST
            64,  // DESERT
            64,  // RED_DESERT
            73,  // TAIGA
            65,  // TUNDRA
            102, // MOUNTAINS
            63,  // SAVANNA
        };
        int biomeAmplitude[(int)BIOME::COUNT] = {
            10,  // PLAINS
            20,  // FOREST
            7,   // DESERT
            8,   // RED_DESERT
            22,  // TAIGA
            12,  // TUNDRA
            105, // MOUNTAINS
            10   // SAVANNA
        };

        float mountainChance    = 0.12f; // fraction of coarse cells rolled as mountains
        float TundraHumidThresh = 0.5f;
        float PlainsHumidThresh = 0.5f;
        float desertHumidThresh = 0.5f;
        float redDesertRarity   = 0.85f; // 1 is rarest
        float tempCold          = 0.40f;
        float tempHot           = 0.60f;

        // Border ZOOM + CELLS
        int  biomeZoomLevels    = 7; // coarse cell
        int  biomeSmoothPasses  = 2;
        int  biomeFuzzyLevels   = 1;

        // Biome seam adjustment
        int biomeBlendRadius    = 16;



        // STRATA & ORES -----------------------------------------------------------------------------------------------
        float strataScale   = 22.0f;   // diorite/granite clump size
        float dioriteThresh = 0.78f;
        float graniteThresh = 0.78f;
        float oreScale      = 9.0f;   // ore clump size
        float coalThresh    = 0.82f;
        float ironThresh    = 0.88f;
        float copperThresh  = 0.85f;


        // WATER
        int   waterLevel    = 63;      // water surface height
        int   riverNoiseOct = 3;
        float riverScale    = 280.0f;  // river "wrinkle" size
        float riverWidth    = 0.05f;   // river half-width
        int   riverDepth    = 8;
        int   pondNoiseOct  = 3;
        float pondScale     = 120.0f;
        float pondThreshold = 0.12f;
        int   pondDepth     = 6;
    };
}
