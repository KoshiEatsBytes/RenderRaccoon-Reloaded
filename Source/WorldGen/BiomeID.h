
#pragma once
#include <cstdint>

namespace WORLDGEN
{
    enum class BIOME : std::uint8_t
    {
        PLAINS,
        FOREST,
        DESERT,
        MESA,
        TAIGA,
        TUNDRA,
        MOUNTAINS,
        SAVANNA,

        COUNT
    };
}
