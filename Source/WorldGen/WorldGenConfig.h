
#pragma once
#include <cstdint>

namespace WORLDGEN
{
    struct WorldGenConfig
    {
        std::uint32_t seed = 456356;

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
        float oreScale      = 11.0f;   // ore clump size
        float coalThresh    = 0.85f;
        float ironThresh    = 0.87f;
        float copperThresh  = 0.86f;
    };
}
