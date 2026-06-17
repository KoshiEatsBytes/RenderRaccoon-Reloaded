
#pragma once
#include <cstdint>

namespace WORLDGEN
{
    enum class BIOME : std::uint8_t
    {
        PLAINS,
        FOREST,
        DESERT,
        RED_DESERT,
        TAIGA,
        TUNDRA,
        MOUNTAINS,
        SAVANNA,

        COUNT
    };
}
