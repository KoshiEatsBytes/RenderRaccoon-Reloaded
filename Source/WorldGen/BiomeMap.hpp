
#pragma once
#include "BiomeID.h"
#include "Noise.hpp"
#include "WorldGenConfig.h"

namespace WORLDGEN
{
    // Distinct salts so each roll is an independent stream
    constexpr uInt32 kSaltTemp  = 811u;
    constexpr uInt32 kSaltHumid = 822u;
    constexpr uInt32 kSaltRare  = 833u;
    constexpr uInt32 kSaltMount = 844u;

    // Coarse-cell biome: random category per cell
    inline BIOME BaseBiome(int _cx, int _cz, const WorldGenConfig& _config)
    {
        // Hash and check for mountains
        if (HashFloat(_cx, _cz, _config.seed + kSaltMount) < _config.mountainChance)
        {
            return BIOME::MOUNTAINS;
        }

        const float temp  = HashFloat(_cx, _cz, _config.seed + kSaltTemp);
        const float humid = HashFloat(_cx, _cz, _config.seed + kSaltHumid);

        // COLD
        if (temp < _config.tempCold)
        {
            // Tundra or taiga
            if (humid < _config.TundraHumidThresh) return BIOME::TUNDRA;
            return BIOME::TAIGA;
        }
        // HOT
        if (temp > _config.tempHot)
        {
            if (HashFloat(_cx, _cz, _config.seed + kSaltRare) > _config.redDesertRarity) {
                return BIOME::RED_DESERT;
            }

            // Desert or savanna
            const float hash = HashFloat(_cx, _cz, _config.seed + kSaltHumid);
            if (hash < _config.desertHumidThresh) return BIOME::DESERT;
            return BIOME::SAVANNA;
        }
        // TEMPERATE
        // Plains or forest
        if (humid < _config.PlainsHumidThresh) return BIOME::PLAINS;
        return BIOME::FOREST;
    }
}