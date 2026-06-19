
#pragma once
#include <cstdint>
#include "Voxels/ChunkData.h"
#include "BiomeID.h"
#include "Voxels/Chunk.h"
#include "Noise.hpp"
#include "Biome.hpp"
#include "WorldGenConfig.h"

namespace WORLDGEN
{
    using BLOCK = RR::CHUNK::BLOCK;

    enum class TREE : std::uint8_t
    {
        NONE,
        OAK,
        SPRUCE_TALL,
        SPRUCE_SMALL,
        ACACIA,

        COUNT
    };

    enum class FLOWERS : std::uint8_t
    {
        NONE,
        ALL,
        POPPY_DANDELION,
        TULIPS,

        COUNT
    };

    // Vegetation biomes hold
    struct BiomeVegTypes
    {
        TREE    tree;
        BLOCK   grass;
        FLOWERS flowers;
        BLOCK   bush;
        bool    cactus;
        bool    boulders;
    };

    // Indexed by biome
    inline constexpr BiomeVegTypes kVegTypes[] = {
        /* PLAINS    */ { TREE::OAK,          BLOCK::SHORT_GRASS,         FLOWERS::ALL,             BLOCK::AIR,       false, false },
        /* FOREST    */ { TREE::OAK,          BLOCK::SHORT_GRASS,         FLOWERS::ALL,             BLOCK::BUSH,           false, false },
        /* DESERT    */ { TREE::NONE,         BLOCK::SHORT_DRY_GRASS,     FLOWERS::NONE,            BLOCK::DEAD_BUSH,      true,  false },
        /* MESA      */ { TREE::NONE,         BLOCK::SHORT_DRY_GRASS,     FLOWERS::NONE,            BLOCK::DEAD_BUSH,      false, false },
        /* TAIGA     */ { TREE::SPRUCE_TALL,  BLOCK::FERN,          FLOWERS::NONE,            BLOCK::DEAD_BUSH,      false, true  },
        /* TUNDRA    */ { TREE::SPRUCE_SMALL, BLOCK::SNOW_SHORT_GRASS,    FLOWERS::TULIPS,          BLOCK::AIR,       false, false },
        /* MOUNTAINS */ { TREE::NONE,         BLOCK::SHORT_GRASS,         FLOWERS::NONE,            BLOCK::AIR,       false, false },
        /* SAVANNA   */ { TREE::ACACIA,       BLOCK::SAVANNA_SHORT_GRASS, FLOWERS::POPPY_DANDELION, BLOCK::AIR,       false, false },
    };
    static_assert(std::size(kVegTypes) == static_cast<std::size_t>(BIOME::COUNT));

    // Helper to get a biome's vegetation type
    inline constexpr const BiomeVegTypes& GetVegTypes(BIOME _biome)
    {
        return kVegTypes[static_cast<std::size_t>(_biome)];
    }

    // Picks the a random flower using the hash and the specified flowerset
    inline BLOCK PickFlower(FLOWERS _set, uInt32 _hash)
    {
        switch (_set)
        {
            case FLOWERS::ALL:
            {
                constexpr BLOCK flower[] = {
                    BLOCK::POPPY,
                    BLOCK::DANDELION,
                    BLOCK::ALLIUM,
                    BLOCK::BLUE_ORCHID,
                    BLOCK::RED_TULIP,
                    BLOCK::ORANGE_TULIP,
                    BLOCK::PINK_TULIP,
                    BLOCK::WHITE_TULIP
                };

                return flower[_hash % 8];
            }
            case FLOWERS::POPPY_DANDELION:
            {
                constexpr BLOCK flower[] = {
                    BLOCK::POPPY,
                    BLOCK::DANDELION
                };

                return flower[_hash % 2];
            }
            case FLOWERS::TULIPS:
            {
                constexpr BLOCK flower[] = {
                    BLOCK::RED_TULIP,
                    BLOCK::ORANGE_TULIP,
                    BLOCK::PINK_TULIP,
                    BLOCK::WHITE_TULIP
                };

                return flower[_hash % 4];
            }
            default: return BLOCK::AIR;
        }
    }

    // Helper to check if a specific block is terracotta
    inline bool IsTerracotta(BLOCK _sample)
    {
        return _sample >= BLOCK::TERRACOTTA && _sample <= BLOCK::BLACK_TERRACOTTA;
    }

    inline void PlaceGroundCover(RR::Chunk& _chunk, int _x, int _surfaceY, int _z,
                                 int _wx, int _wz, BIOME _biome, BLOCK _surface,
                                 const WorldGenConfig& _config)
    {
        using namespace RR::CHUNK;

        const int y = _surfaceY + 1;

        // check if out of bounds
        if (y >= kSizeY) return;

        const BiomeVegTypes& types     = GetVegTypes(_biome);
        const BiomeVeg&      details   = _config.biomeVegetation[static_cast<int>(_biome)];
        const uInt32         seed      = _config.seed;
        const bool           onSurface = _surface == GetBiome(_biome).surface;

        // Cactuses 2 to 4 blocks tall
        const float cactusHash = HashFloat(_wx, _wz, seed + 1001u);
        if (types.cactus && onSurface && cactusHash < details.cactus)
        {
            // hash returns from 0 to 2
            const int height = 2 + static_cast<int>(HashFloat(_wx, _wz, seed + 1002u) * 3.0f);

            for (int i = 0; i < height && y + i < kSizeY; ++i)
            {
                _chunk.Set(_x, y + i, _z, static_cast<BlockId>(BLOCK::CACTUS));
            }
            // close early
            return;
        }

        // BUSH / DEAD BUSH
        const float hashBush = HashFloat(_wx, _wz, seed + 1003u);
        if (types.bush != BLOCK::AIR && hashBush < details.bush)
        {
            if (onSurface || (_biome == BIOME::MESA && IsTerracotta(_surface)))
            {
                _chunk.Set(_x, y, _z, static_cast<BlockId>(types.bush));
                return;
            }
        }

        // FLOWERS
        const float hashFlower = HashFloat(_wx, _wz, seed + 1004u);
        if (types.flowers != FLOWERS::NONE && onSurface && hashFlower < details.flower)
        {
            const uInt32 pick = HashU32(_wx, _wz, seed + 1005u);
            _chunk.Set(_x, y, _z, static_cast<BlockId>(PickFlower(types.flowers, pick)));
            return;
        }

        // GRASS and TALL GRASS
        const float hashGrass = HashFloat(_wx, _wz, seed + 1006u);
        if (types.grass != BLOCK::AIR && onSurface && hashGrass < details.grass)
        {
            const float hashTallGrass = HashFloat(_wx, _wz, seed + 1007u);

            if (details.tallGrass > 0.0f && y + 1 < kSizeY && hashTallGrass < details.tallGrass)
            {
                // places tall grass as column
                _chunk.Set(_x, y,     _z, static_cast<BlockId>(BLOCK::TALL_GRASS_LOWER));
                _chunk.Set(_x, y + 1, _z, static_cast<BlockId>(BLOCK::TALL_GRASS_UPPER));
            }
            else
            {
                _chunk.Set(_x, y, _z, static_cast<BlockId>(types.grass));
            }
            return;
        }
    }
}