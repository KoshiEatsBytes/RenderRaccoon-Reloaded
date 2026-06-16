
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

    // Shoudnt amplitude be tweakeable from WorldGenConfig?
    inline constexpr BiomeParams kBiomes[] = {
        /* PLAINS     */ {24, BLOCK::GRASS, BLOCK::DIRT},
        /* FOREST     */ {32, BLOCK::GRASS, BLOCK::DIRT},
        /* DESERT     */ {14, BLOCK::SAND, BLOCK::SANDSTONE},
        /* RED DESERT */ {16, BLOCK::RED_SAND, BLOCK::RED_SANDSTONE},
        /* TAIGA      */ {40, BLOCK::PODZOL, BLOCK::DIRT},
        /* TUNDRA     */ {20, BLOCK::SNOWY_GRASS, BLOCK::DIRT}
    };
    static_assert(std::size(kBiomes) == static_cast<std::size_t>(BIOME::COUNT));

    inline const BiomeParams& GetBiome(BIOME _biome)
    {
        return kBiomes[static_cast<std::size_t>(_biome)];
    }

    inline BIOME SelectBiome(int _wx, int _wz, const WorldGenConfig& _config)
    {
        const int   tempOct = _config.temperatureOctaves;
        const int   humOct  = _config.humidityOctaves;
        const int   rarOct  = _config.rarityOctaves;

        const float temperature = FBM(_wx / _config.temperatureScale, _wz / _config.temperatureScale, _config.seed + 101u, tempOct);
        const float humidity    = FBM(_wx / _config.humidityScale,    _wz / _config.humidityScale,    _config.seed + 202u, humOct);

        // if cold
        if (temperature < _config.tempCold)
        {
            // split on humidity
            if (humidity < _config.TundraHumidThresh) return BIOME::TUNDRA;
            return BIOME::TAIGA;
        }
        // if hot
        if (temperature > _config.tempHot)
        {
            const float rarity  = FBM(_wx / _config.temperatureScale, _wz / _config.temperatureScale, _config.seed + 303u, 2);

            if (rarity > _config.redDesertRarity) return BIOME::RED_DESERT;

            return BIOME::DESERT;
        }
        // if temperate
        if (humidity < _config.PlainsHumidThresh) return BIOME::PLAINS;

        return BIOME::FOREST;
    }

}
