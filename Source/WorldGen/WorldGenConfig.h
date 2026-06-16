
#pragma once
#include <cstdint>

namespace WORLDGEN
{
    struct WorldGenConfig
    {
        using uInt32 = std::uint32_t;

        uInt32 seed = 456356;

        // BASE TERRAIN
        float heightScale   = 128.f; // noise in block spacing
        int   heightBase    = 64;    // Sea level
        int   heightAmp     = 40;    // Hill height range
        int   heightOctaves = 5;
        int   dirtDepth     = 4;     // Dirt thickness under grass

        // BIOMES
        float temperatureScale = 384.0f; // broad climate zones
        float humidityScale    = 120.0f; // local moisture patches

        int temperatureOctaves = 3;
        int humidityOctaves    = 3;
        int rarityOctaves      = 2;

        float TundraHumidThresh = 0.5f;
        float PlainsHumidThresh = 0.5f;
        float redDesertRarity   = 0.85f; // 1 is rarest
        float tempCold          = 0.40f;
        float tempHot           = 0.60f;
        float temperate         = 0.5f;

        // STRATA & ORES
        float strataScale   = 22.0f;   // diorite/granite clump size
        float dioriteThresh = 0.78f;
        float graniteThresh = 0.78f;
        float oreScale      = 9.0f;   // ore clump size
        float coalThresh    = 0.82f;
        float ironThresh    = 0.88f;
        float copperThresh  = 0.85f;

        // CLIFFS
        int    cliffOctaves = 3;
        float  cliffScale   = 200.0f;   // size of cliff
        int    cliffStep    = 28;       // height per level
        int    cliffLevels  = 3;        // plateau tiers
        uInt32 cliffSub     = 707u;

        // WATER
        int   waterLevel    = 70;      // water surface height
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
