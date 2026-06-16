
#pragma once
#include <cstdint>
#include "Voxels/ChunkData.h"
#include "Noise.hpp"
#include "WorldGenConfig.h"

namespace WORLDGEN
{
    using BLOCK = RR::CHUNK::BLOCK;

    enum class BIOME : std::uint8_t
    {
        PLAINS,
        FOREST,
        DESERT,
        RED_DESERT,
        TAIGA,
        TUNDRA,

        COUNT
    };

    struct BiomeParams
    {
        int   amplitude;  // Height variation for this biome
        BLOCK surface;    // Top block for biome
        BLOCK subsurface; // Band beneath the surface
    };
}
