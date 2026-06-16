
#pragma once
#include <cstdint>

namespace WORLDGEN
{
    struct WorldGenConfig
    {
        std::uint32_t seed = 456356;

        // BASE TERRAIN
        float heightScale   = 128.f;    // noise in block spacing
        int   heightBase    = 64;       // Sea level
        int   heightAmp     = 40;       // Hill height range
        int   heightOctaves = 5;
        int   dirtDepth     = 4;        // Dirt thickness under grass
    };
}
