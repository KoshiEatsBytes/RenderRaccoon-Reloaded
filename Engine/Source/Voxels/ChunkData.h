
#pragma once
#include <cstdint>
#include <array>
#include <cstddef>
#include <functional>

namespace RR::CHUNK
{
    using BlockId = std::uint8_t;

    inline constexpr int kSizeX = 16;
    inline constexpr int kSizeZ = 16;
    inline constexpr int kSizeY = 256;
    // A chunk is 64kb of memory
    inline constexpr int kVoxelsPerChunk = kSizeX * kSizeZ * kSizeY;

    // BLOCKS ----------------------------------------------------------------------------------------------------------

    // Block Repository
    enum class BLOCK : BlockId
    {
        AIR = 0,

        GRASS,
        DIRT,
        STONE,
        BEDROCK,
        DIORITE,
        GRANITE,
        SAND,
        SNOW,
        WATER,
        OAKLOG,
        LEAVES,

        COUNT
    };

    // block texture
    enum BLOCKTEX : std::uint16_t
    {
        GRASS_TOP,
        GRASS_SIDE,
        DIRT,
        STONE,
        BEDROCK,
        DIORITE,
        GRANITE,
        SAND,
        SNOW,
        WATER,
        OAK_LOG_SIDE,
        OAK_LOG_END,
        OAK_LEAVES,
        COUNT
    };

    // Specifies which faces can be seamlessly rotated on the Y axis
    inline constexpr bool kTexRotatable[] =
        {
        true,  // GRASS_TOP
        false, // GRASS_SIDE
        true,  // DIRT
        false, // STONE
        false, // BEDROCK
        false, // DIORITE
        false, // GRANITE
        true,  // SAND
        true,  // SNOW
        false, // WATER
        false, // OAK SIDE
        false, // OAK END
        false, // OAK LEAVES
        };
    static_assert(std::size(kTexRotatable) == static_cast<std::size_t>(BLOCKTEX::COUNT),
        "kTexRotatable needs exactly one entry per BlockTex");

    inline constexpr bool IsTexRotatable(BLOCKTEX _tex)
    {
        return kTexRotatable[static_cast<std::size_t>(_tex)];
    }

    enum class FACE : std::uint8_t
    {
        EAST,
        WEST,
        UP,
        DOWN,
        SOUTH,
        NORTH
    };

    struct BlockInfo
    {
        bool solid = true;
        std::uint16_t faceLayer[6] = {};
    };

    // Face order is EAST, WEST, UP, DOWN, SOUTH, NORT
    constexpr BlockInfo UniformBlock(BLOCKTEX _tex, bool _solid = true)
    {
        return { _solid, { _tex, _tex, _tex, _tex, _tex, _tex } };
    }
    constexpr BlockInfo SidedBlock(BLOCKTEX _side, BLOCKTEX _top, BLOCKTEX _bottom)
    {
        return { true, { _side, _side, _top, _bottom, _side, _side } };
    }

    // Block info table
    inline constexpr std::array<BlockInfo, static_cast<std::size_t>(BLOCK::COUNT)> kBlocks = {
        {
            /* Air     */ { false, {} },
            /* Grass   */ SidedBlock  (GRASS_SIDE, GRASS_TOP, DIRT),
            /* Dirt    */ UniformBlock(DIRT),
            /* Stone   */ UniformBlock(STONE),
            /* Bedrock */ UniformBlock(BEDROCK),
            /* Diorite */ UniformBlock(DIORITE),
            /* Granite */ UniformBlock(GRANITE),
            /* Sand    */ UniformBlock(SAND),
            /* Snow    */ UniformBlock(SNOW),
            /* Water   */ UniformBlock(WATER),
            /* OakLog  */ SidedBlock  (OAK_LOG_SIDE, OAK_LOG_END, OAK_LOG_END),
            /* Leaves  */ UniformBlock(OAK_LEAVES),
        }
    };

    // Generate compiler error if a block is not added to the info table
    static_assert(kBlocks.size() == static_cast<std::size_t>(BLOCK::COUNT));

    // Fetch info table
    inline const BlockInfo& GetBlockInfo(BLOCK _block)
    {
        return kBlocks[static_cast<std::size_t>(_block)];
    }

    // Fetch info table
    inline const BlockInfo& GetBlockInfo(BlockId _block)
    {
        return kBlocks[_block];
    }

    // Integer has for block rotation, deterministic on every machine
    inline std::uint32_t Hash(int _wx, int _wz)
    {
        std::uint32_t hash = static_cast<std::uint32_t>(_wx) * 0x9e3779b1u
                           ^ static_cast<std::uint32_t>(_wz) * 0x85ebca77u;

        hash ^= hash >> 16;
        hash *= 0x7feb352du;
        hash ^= hash >> 15;
        hash *= 0x846ca68bu;
        hash ^= hash >> 16;

        return hash;
    }

    // 90-step rotation index for faces
    inline int FaceRotation(int _wx, int _wz)
    {
        return static_cast<int>(Hash(_wx, _wz) & 3u);
    }

    // CHUNKS ----------------------------------------------------------------------------------------------------------

    // Current state of a chunk
    enum class STATE : std::uint8_t
    {
        EMPTY,
        GENERATED,
        MESHED
    };

    struct Coord
    {
        int x = 0;
        int z = 0;

        // comparison operator
        bool operator==(const Coord & _out) const
        {
            return x == _out.x && z == _out.z;
        }
    };

    struct CoordHash
    {
        std::size_t operator() (const Coord& _chunk) const
        {
            // merge two int32 into an int64
            std::uint64_t key =
                static_cast<std::uint64_t>(static_cast<std::uint32_t>(_chunk.x)) << 32 |
                static_cast<std::uint32_t>(_chunk.z);

            return std::hash<std::uint64_t>{}(key);
        }
    };

    // walk a column to return index
    inline int Index(int _x, int _y, int _z)
    {
        return (_z * kSizeX + _x) * kSizeY + _y;
    }
}
