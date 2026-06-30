
#pragma once
#include <cstdint>
#include <algorithm>
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
        BLOCK   canopy;   
    };

    // tree margin
    constexpr int kTreeMargin = 6;  

    // Indexed by biome
    inline constexpr BiomeVegTypes kVegTypes[] = {
        /* PLAINS    */ { TREE::OAK,          BLOCK::SHORT_GRASS,         FLOWERS::ALL,             BLOCK::AIR,        false, false, BLOCK::AIR           },
        /* FOREST    */ { TREE::OAK,          BLOCK::SHORT_GRASS,         FLOWERS::ALL,             BLOCK::BUSH,       false, false, BLOCK::CANOPY_OAK    },
        /* DESERT    */ { TREE::NONE,         BLOCK::SHORT_DRY_GRASS,     FLOWERS::NONE,            BLOCK::DEAD_BUSH,  true,  false, BLOCK::AIR           },
        /* MESA      */ { TREE::NONE,         BLOCK::SHORT_DRY_GRASS,     FLOWERS::NONE,            BLOCK::DEAD_BUSH,  false, false, BLOCK::AIR           },
        /* TAIGA     */ { TREE::SPRUCE_TALL,  BLOCK::FERN,                FLOWERS::NONE,            BLOCK::DEAD_BUSH,  false, true,  BLOCK::CANOPY_SPRUCE },
        /* TUNDRA    */ { TREE::SPRUCE_SMALL, BLOCK::SNOW_SHORT_GRASS,    FLOWERS::TULIPS,          BLOCK::AIR,        false, false, BLOCK::CANOPY_TUNDRA },
        /* MOUNTAINS */ { TREE::NONE,         BLOCK::SHORT_GRASS,         FLOWERS::NONE,            BLOCK::AIR,        false, false, BLOCK::AIR           },
        /* SAVANNA   */ { TREE::ACACIA,       BLOCK::SAVANNA_SHORT_GRASS, FLOWERS::POPPY_DANDELION, BLOCK::AIR,        false, false, BLOCK::CANOPY_ACACIA },
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

    // tree clumping, low freq noise
    inline float TreeClumpField(int _wx, int _wz, const WorldGenConfig& _config)
    {
        float sx = static_cast<float>(_wx);
        float sz = static_cast<float>(_wz);

        // domain-warp so grove/clearing edges are sinuous, not round blobs
        if (_config.clumpWarp > 0.0f)
        {
            const float px = static_cast<float>(_wx) / _config.clumpScale;
            const float pz = static_cast<float>(_wz) / _config.clumpScale;
            
            sx += (FBM(px, pz, _config.seed + 1021u, 2) - 0.5f) * 2.0f * _config.clumpWarp;
            sz += (FBM(px, pz, _config.seed + 1022u, 2) - 0.5f) * 2.0f * _config.clumpWarp;
        }

        const float noise = FBM(sx / _config.clumpScale, sz / _config.clumpScale,
                            _config.seed + 1020u, 3);
        const float denom = _config.clumpThick - _config.clumpClear;

        float density = 0.0f;
        if (denom > 1e-5f) {
            density = std::clamp((noise - _config.clumpClear) / denom, 0.0f, 1.0f);
        }
        else {
            density = noise >= _config.clumpThick ? 1.0f : 0.0f;
        }

        return Smooth(density) * _config.clumpPeak;
    }

    // biome blend between uniform scatter 
    inline float TreeClump(int _wx, int _wz, BIOME _biome, const WorldGenConfig& _config)
    {
        // Only clump if enabled
        if (!_config.clumpEnabled) return 1.0f;

        const float amount = _config.biomeVegetation[static_cast<int>(_biome)].clumpAmount;
        if (amount <= 0.0f) return 1.0f;

        return Lerp(1.0f, TreeClumpField(_wx, _wz, _config), amount);
    }

    inline bool TreeSpawnRoll(int _wx, int _wz, BIOME _biome, const WorldGenConfig& _config)
    {
        if (GetVegTypes(_biome).tree != TREE::NONE                 &&
            HashFloat(_wx, _wz, _config.seed + 1010u)     <
            _config.biomeVegetation[static_cast<int>(_biome)].tree *
            TreeClump(_wx, _wz, _biome, _config))
        {
            return true;
        }
        return false;
    }

    // To keep or not to keep proxies, that is the question *philosophy pose*
    inline float ProxyKeep(int _level, const WorldGenConfig& _config)
    {
        return _config.proxyKeepBase * std::pow(_config.proxyKeepFalloff, static_cast<float>(_level - 1));
    }

    // Tunable trunk height, deterministic pick min/max
    inline int PickHeight(uInt32 _hash, int _min, int _max)
    {
        if (_max < _min) _max = _min;
        const uInt32 span = static_cast<uInt32>(_max - _min) + 1u;
        return _min + static_cast<int>(_hash % span);
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

        // Cactuses 3 to 5 blocks tall
        const float cactusHash = HashFloat(_wx, _wz, seed + 1001u);
        if (types.cactus && onSurface && cactusHash < details.cactus)
        {
            // hash returns from 0 to 2
            const int height = 3 + static_cast<int>(HashFloat(_wx, _wz, seed + 1002u) * 3.0f);

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
    inline void StampOak(RR::Chunk& _chunk, int _lx, int _rootY, int _lz, uInt32 _hash, int _minH, int _maxH)
    {
        using namespace RR::CHUNK;

        const int trunkHeight = PickHeight(_hash, _minH, _maxH);
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

    // Generate tall spruce 2x2 trunk, wide tiered skirts
    inline void StampSpruceTall(RR::Chunk& _chunk, int _lx, int _rootY, int _lz, uInt32 _hash, int _minH, int _maxH)
    {
        using namespace RR::CHUNK;

        const int trunkHeight = PickHeight(_hash, _minH, _maxH);
        const int topY        = _rootY + trunkHeight;

        // 2x2 trunk
        for (int y = _rootY + 1; y <= topY; ++y)
        {
            SetClipped(_chunk, _lx,     y, _lz,     BLOCK::SPRUCE_LOG);
            SetClipped(_chunk, _lx + 1, y, _lz,     BLOCK::SPRUCE_LOG);
            SetClipped(_chunk, _lx,     y, _lz + 1, BLOCK::SPRUCE_LOG);
            SetClipped(_chunk, _lx + 1, y, _lz + 1, BLOCK::SPRUCE_LOG);
        }

        // pointed tip over the 2x2
        for (int tz = 0; tz <= 1; ++tz)
        {
            for (int tx = 0; tx <= 1; ++tx)
            {
                SetClippedIfAir(_chunk, _lx + tx, topY + 1, _lz + tz, BLOCK::SPRUCE_LEAVES);
            }
        }

        // tiered skirts around the trunk
        const int canopyBottom = _rootY + 6;
        for (int y = topY; y >= canopyBottom; --y)
        {
            const int depth = topY - y;                       
            int radius      = depth / 2;  

            // fallbacks to prevent weird shapes
            if (radius > 5) radius = 5;                        
            if (depth % 3 == 0 && radius > 0) radius -= 1;      

            for (int dz = -radius; dz <= radius + 1; ++dz)
            {
                for (int dx = -radius; dx <= radius + 1; ++dx)
                {
                    // distance outside, symmetric skirt
                    const int ox = (dx < 0) ? -dx : (dx > 1 ? dx - 1 : 0);
                    const int oz = (dz < 0) ? -dz : (dz > 1 ? dz - 1 : 0);

                    if (ox * ox + oz * oz <= radius * radius)
                    {
                        SetClippedIfAir(_chunk, _lx + dx, y, _lz + dz, BLOCK::SPRUCE_LEAVES);
                    }
                }
            }
        }
    }

    // generate small spruce at coords
    inline void StampSpruceSmall(RR::Chunk& _chunk, int _lx, int _rootY, int _lz, uInt32 _hash, int _minH, int _maxH)
    {
        using namespace RR::CHUNK;

        // trunk height range is per-biome tunable (default 6 to 8)
        const int trunkHeight = PickHeight(_hash, _minH, _maxH);
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
    inline void StampAcacia(RR::Chunk& _chunk, int _lx, int _rootY, int _lz, uInt32 _hash, int _minH, int _maxH)
    {
        using namespace RR::CHUNK;

        // for acacia create a two layer tree
        auto plate = [&](int _cx, int _cy, int _cz)
        {
            for (int dz = -3; dz <= 3; ++dz)
            {
                for (int dx = -3; dx <= 3; ++dx)
                {
                    const int dist2 = dx * dx + dz * dz;

                    if (dist2 <= 9) SetClippedIfAir(_chunk, _cx + dx, _cy,     _cz + dz, BLOCK::ACACIA_LEAVES);
                    if (dist2 <= 4) SetClippedIfAir(_chunk, _cx + dx, _cy + 1, _cz + dz, BLOCK::ACACIA_LEAVES);
                }
            }
        };

        // Acacia has diagonal tree braches before leaves 
        auto branch = [&](int _bx, int _bz, int _by, int _dirX, int _dirZ, int _len)
        {
            for (int i = 0; i < _len; ++i)
            {
                _bx += _dirX; 
                _bz += _dirZ; 
                _by += 1;

                SetClipped(_chunk, _bx, _by, _bz, BLOCK::ACACIA_LOG);
            }
            plate(_bx, _by, _bz);
        };

        // lower straight trunk, then bend
        const int baseH = PickHeight(_hash, _minH, _maxH);
        const int bendY = _rootY + baseH;

        for (int y = _rootY + 1; y <= bendY; ++y)
        {
            SetClipped(_chunk, _lx, y, _lz, BLOCK::ACACIA_LOG);
        }

        const int DX[4] = { 1, -1, 0, 0 };
        const int DZ[4] = { 0, 0, 1, -1 };

        // main branch 
        const int d0 = static_cast<int>((_hash >> 8) & 3u);
        branch(_lx, _lz, bendY, DX[d0], DZ[d0], 2 + static_cast<int>((_hash >> 4) & 1u));

        // half chanche of a second shorter branch
        if (((_hash >> 12) & 1u) == 0u)
        {
            const int d1 = (d0 + 1 + static_cast<int>((_hash >> 13) & 1u)) & 3;
            
            branch(_lx, _lz, bendY, DX[d1], DZ[d1], 1 + static_cast<int>((_hash >> 14) & 1u));
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