
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

        // TERRAIN SHAPE -----------------------------------------------------------------------------------------------
        // Warping
        bool  warpEnabled   = true;
        float warpScale     = 200.0f;
        float warpAmp       = 100.f;
        int   warpOctaves   = 3;
        int   warpLevels    = 2;  // 1 = single warp 2 = recursive
        // Terracing
        bool  detailEnabled  = true;
        float detailScale    = 32.0f;  // high freq = roughens edges
        float detailAmp      = 2.0f;   // blocks of edge jitter
        int   detailOctaves  = 3;

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
            35,  // TAIGA
            12,  // TUNDRA
            105, // MOUNTAINS
            10   // SAVANNA
        };

        float mountainChance    = 0.2f; // fraction of coarse cells rolled as mountains
        float TundraHumidThresh = 0.5f;
        float PlainsHumidThresh = 0.5f;
        float desertHumidThresh = 0.5f;
        float redDesertRarity   = 0.835f; // 1 is rarest
        float tempCold          = 0.40f;
        float tempHot           = 0.60f;

        // Border ZOOM + CELLS
        int  biomeZoomLevels    = 8; // coarse cell
        int  biomeSmoothPasses  = 2;
        int  biomeFuzzyLevels   = 1;

        // Biome seam adjustment
        int   biomeBlendRadius = 20;
        float mountainCurve    = 1.5f;

        // MOUNTAIN SURFACE
        int   snowLine        = 150;   // Y above which mountain surface is snow
        int   mtnGrassLine    = 112;   // below this grassy foothills
        float snowJitterScale = 40.0f; // wavelength of the wavy snow/grass line
        float snowJitterAmp   = 12.0f; // blocks the line wobbles
        // Ridges
        bool  ridgeMountains = true;
        float ridgeAmp       = 1.f;    // 0 = round, 1 = full ridge



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
