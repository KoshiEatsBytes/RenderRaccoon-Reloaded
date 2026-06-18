
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
        bool  useGradientNoise = true; // use perlin
        int   waterLevel       = 58;      // water surface height
        float heightScale      = 128.f; // noise in block spacing
        int   heightOctaves    = 5;
        int   dirtDepth        = 4;     // Dirt thickness under grass

        // TERRAIN SHAPE -----------------------------------------------------------------------------------------------
        // Warping
        bool  warpEnabled   = true;
        float warpScale     = 400.0f;
        float warpAmp       = 100.f;
        int   warpOctaves   = 3;
        int   warpLevels    = 2;  // 1 = single warp 2 = recursive
        // Terracing
        bool  detailEnabled  = true;
        float detailScale    = 48.0f;  // high freq = roughens edges
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
            15,  // FOREST
            7,   // DESERT
            8,   // RED_DESERT
            35,  // TAIGA
            12,  // TUNDRA
            105, // MOUNTAINS
            10   // SAVANNA
        };

        float mountainChance    = 0.2f; // fraction of coarse cells rolled as mountains
        float tundraHumidThresh = 0.5f;
        float plainsHumidThresh = 0.5f;
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
        float ridgeStrength  = 1.f;    // 0 = round, 1 = full ridge

        // RIVERS ------------------------------------------------------------------------------------------------------

        bool  riverWarpEnabled   = false;
        int   riverDepth         = 6;       // deep inner-channel depth below riverLevel
        int   riverShelfDepth    = 2;       // shallow outer-shelf depth below riverLevel
        float channelThreshold   = 0.525f;    // valleyTerr above this carves the deep channel (higher = narrower)
        int   riverLevel         = 63;
        int   riverMaxHeight     = 120;      // Y where rivers start fading out
        float riverFade          = 10.f;    // fade range in blocks
        float riverWarpScale     = 80.f;    // river meander wavelength
        float riverWarpAmp       = 15.0f;   // how far the channel wanders
        float riverValleyWidth   = 0.085f;
        float riverScale         = 200.0f;  // river "wrinkle" size
        int   riverNoiseOct      = 3;
        bool  iceEnabled         = false;   // freeze rivers/seas in cold biomes
        bool  taigaRivers        = false;   // allow rivers through taiga (off: taiga's height glitches them)
        bool  tributariesEnabled = false;
        float tribScale          = 90.0f;   // riverScale = more, smaller branches
        float tribValleyWidth    = 0.05f;   // riverValleyWidth = narrower
        float tribStrength       = 0.6f;
        float beachBand          = 0.25f;   // how far past the water edge the beach reaches
        float beachSandChance    = 0.70f;   // scatter sand vs the normal block (1.0 = solid sand)
        bool  desertRiverGrass   = true;    // desert river banks scatter grass


        // STRATA & ORES -----------------------------------------------------------------------------------------------
        float strataScale   = 22.0f;   // diorite/granite clump size
        float dioriteThresh = 0.78f;
        float graniteThresh = 0.78f;
        float oreScale      = 9.0f;    // ore clump size
        float coalThresh    = 0.82f;
        float ironThresh    = 0.88f;
        float copperThresh  = 0.85f;
        
    };
}
