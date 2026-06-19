
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

    // tree margin
    constexpr int kTreeMargin = 3;

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

    // Used for tree logs... mostly
    inline void SetClipped(RR::Chunk& _chunk, int _x, int _y, int _z, BLOCK _block)
    {
        using namespace RR::CHUNK;

        if (_x < 0 || _x >= kSizeX || _z < 0 || _z >= kSizeZ || _y < 0 || _y >= kSizeY) return;

        // If valid spawn log or boulder
        _chunk.Set(_x, _y, _z, static_cast<BlockId>(_block));
    }

    // Used for tree leaves... mostly
    inline void SetClippedIfAir(RR::Chunk& _chunk, int _x, int _y, int _z, BLOCK _block)
    {
        using namespace RR::CHUNK;
        if (_x < 0 || _x >= kSizeX || _z < 0 || _z >= kSizeZ || _y < 0 || _y >= kSizeY) return;

        // Foliage sticsk to solids
        if (_chunk.At(_x, _y, _z) != static_cast<BlockId>(BLOCK::AIR)) return;

        _chunk.Set(_x, _y, _z, static_cast<BlockId>(_block));
    }

    // Generate oak at coords
    inline void StampOak(RR::Chunk& _chunk, int _lx, int _rootY, int _lz, uInt32 _hash)
    {
        using namespace RR::CHUNK;

        // trunk can be 5 to 7 tall
        const int trunkHeight = 5 + static_cast<int>(_hash % 4u);
        const int topY        = _rootY + trunkHeight;

        // Trunk first
        for (int y = _rootY + 1; y <= topY; ++y)
        {
            SetClipped(_chunk, _lx, y, _lz, BLOCK::OAKLOG);
        }

        // canopy, fill around the logs
        for (int dy = -2; dy <= 1; ++dy)
        {
            const int radius = (dy <= -1) ? 2 : 1;
            const int height = topY + dy;

            for (int dz = -radius; dz <= radius; ++dz)
            {
                for (int dx = -radius; dx <= radius; ++dx)
                {
                    // round the 5x5 shape
                    const bool corner2   = radius == 2 && std::abs(dx) == 2 && std::abs(dz) == 2;
                    // plus shaped tree top
                    const bool capCorner = dy == 1 && std::abs(dx) == 1 && std::abs(dz) == 1;

                    if (corner2 || capCorner) continue;

                    SetClippedIfAir(_chunk, _lx + dx, height, _lz + dz, BLOCK::LEAVES);
                }
            }

        }
    }

    // Generate tall spruce at coords
    inline void StampSpruceTall(RR::Chunk& _chunk, int _lx, int _rootY, int _lz, uInt32 _hash)
    {
        using namespace RR::CHUNK;

        // tall spruce is 10 to 14
        const int trunkHeight = 10 + static_cast<int>(_hash % 5u);
        const int topY        = _rootY + trunkHeight;

        for (int y =_rootY + 1; y <= topY; ++y)
        {
            SetClipped(_chunk, _lx, y, _lz, BLOCK::SPRUCE_LOG);
        }
        SetClippedIfAir(_chunk, _lx, topY + 1, _lz, BLOCK::SPRUCE_LEAVES);

        for (int y = topY; y >= _rootY + 3; --y)
        {
            // spruce customization knob
            const int depth  = topY - y;
            const int radius = (depth % 3 == 0) ? 1 : 2;

            for (int dz=-radius; dz<=radius; ++dz)
            {
                for (int dx=-radius; dx<=radius; ++dx)
                {
                    // roundish tree skirt
                    if (dx*dx + dz*dz <= radius*radius)
                    {
                        SetClippedIfAir(_chunk, _lx+dx, y, _lz+dz,  BLOCK::SPRUCE_LEAVES);
                    }
                }
            }

        }
    }

    // generate small spruce at coords
    inline void StampSpruceSmall(RR::Chunk& _chunk, int _lx, int _rootY, int _lz, uInt32 _hash)
    {
        using namespace RR::CHUNK;

        // Small spruce are 4 to 6 tall
        const int trunkHeight = 4 + static_cast<int>(_hash % 3u);
        const int topY        = _rootY + trunkHeight;

        for (int y=_rootY+1; y<=topY; ++y)
        {
            SetClipped(_chunk, _lx, y, _lz, BLOCK::SPRUCE_LOG);
        }
        SetClippedIfAir(_chunk, _lx, topY + 1, _lz, BLOCK::TUNDRA_SPRUCE_LEAVES);

        for (int y = topY; y >= _rootY + 2; --y)
        {
            // compact 2 tier cone
            const int depth = topY - y;
            const int radius = (depth % 2 == 0) ? 1 : 2;

            for (int dz = -radius; dz <= radius; ++dz)
            {
                for (int dx = -radius; dx <= radius; ++dx)
                {
                    if (dx * dx + dz * dz <= radius * radius)
                    {
                        SetClippedIfAir(_chunk, _lx + dx, y, _lz + dz, BLOCK::TUNDRA_SPRUCE_LEAVES);
                    }
                }
            }

        }
    }

    // generate acacia tree at coords
    inline void StampAcacia(RR::Chunk& _chunk, int _lx, int _rootY, int _lz, uInt32 _hash)
    {
        using namespace RR::CHUNK;

        // acacias 5 to 7
        const int trunkHeight = 5 + static_cast<int>(_hash % 3u);
        const int topY        = _rootY + trunkHeight;

        for (int y = _rootY + 1; y <= topY; ++y)
        {
            SetClipped(_chunk, _lx, y, _lz, BLOCK::ACACIA_LOG);
        }

        // flat 2 layer plate
        for (int dz = -3; dz <= 3; ++dz)
        {
            for (int dx = -3; dx <= 3; ++dx)
            {
                const int dist2 = dx * dx + dz * dz;
                // upper ring and lower ring
                if (dist2 <= 9) SetClippedIfAir(_chunk, _lx + dx, topY, _lz + dz, BLOCK::ACACIA_LEAVES);
                if (dist2 <= 4) SetClippedIfAir(_chunk, _lx + dx, topY + 1, _lz + dz, BLOCK::ACACIA_LEAVES);
            }
        }
    }

    // Generates mossy cobblestone blob at given coords
    inline void StampBoulder(RR::Chunk& _chunk, int _lx, int _rootY, int _lz, uInt32 _hash)
    {
        using namespace RR::CHUNK;

        const int radius = 1 + static_cast<int>(_hash % 2u);

        for (int dy = 0; dy <= radius; ++dy)
        {
            for (int dz = -radius; dz <= radius; ++dz)
            {
                for (int dx = -radius; dx <= radius; ++dx)
                {
                    if (dx * dx + dy * dy + dz * dz <= radius * radius + 1)
                    {
                        SetClipped(_chunk, _lx+dx, _rootY+dy, _lz+dz, BLOCK::MOSSY_COBBLESTONE);
                    }
                }
            }

        }

    }
}