
#pragma once
#include <cstdint>
#include "Voxels/ChunkData.h"
#include "Noise.hpp"
#include "BiomeID.h"
#include "WorldGenConfig.h"

namespace WORLDGEN
{
    using BLOCK = RR::CHUNK::BLOCK;

    struct BiomeParams
    {
        BLOCK surface;    // Top block for biome
        BLOCK subsurface; // Band beneath the surface
        bool  cliffEligible;
    };

    // Biomes presets
    inline constexpr BiomeParams kBiomes[] = {
        /* PLAINS     */ { BLOCK::GRASS,         BLOCK::DIRT,          false },
        /* FOREST     */ { BLOCK::GRASS,         BLOCK::DIRT,          false },
        /* DESERT     */ { BLOCK::SAND,          BLOCK::SANDSTONE,     false },
        /* MESA       */ { BLOCK::RED_SAND,      BLOCK::RED_SANDSTONE, false },
        /* TAIGA      */ { BLOCK::PODZOL,        BLOCK::DIRT,          false },
        /* TUNDRA     */ { BLOCK::SNOWY_GRASS,   BLOCK::DIRT,          false },
        /* MOUNTAINS  */ { BLOCK::GRASS,         BLOCK::STONE,         true  },
        /* SAVANNA    */ { BLOCK::SAVANNA_GRASS, BLOCK::DIRT,          false },
    };
    static_assert(std::size(kBiomes) == static_cast<std::size_t>(BIOME::COUNT));

    inline constexpr BLOCK kMesaPalette[] = {
        BLOCK::TERRACOTTA, BLOCK::TERRACOTTA, BLOCK::TERRACOTTA, BLOCK::ORANGE_TERRACOTTA,
        BLOCK::TERRACOTTA, BLOCK::TERRACOTTA, BLOCK::YELLOW_TERRACOTTA, BLOCK::TERRACOTTA,
        BLOCK::TERRACOTTA, BLOCK::WHITE_TERRACOTTA, BLOCK::TERRACOTTA, BLOCK::BROWN_TERRACOTTA,
        BLOCK::TERRACOTTA, BLOCK::TERRACOTTA, BLOCK::RED_TERRACOTTA, BLOCK::LIGHT_GRAY_TERRACOTTA,
    };

    inline const BiomeParams& GetBiome(BIOME _biome)
    {
        return kBiomes[static_cast<std::size_t>(_biome)];
    }
}
