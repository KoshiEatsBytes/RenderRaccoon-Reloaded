
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

    // Current state of a chunk
    enum class State : std::uint8_t
    {
        EMPTY,
        GENERATED,
        MESHED
    };

    // Block Repository
    enum class Block : BlockId
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
    enum BlockTex : std::uint16_t
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

    enum class Face : std::uint8_t
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

    // Block info table
    inline constexpr std::array<BlockInfo, static_cast<std::size_t>(Block::COUNT)> kBlocks = {
        {
            /* Air     */ { false, {} },
            /* Grass   */ { true,  {} },
            /* Dirt    */ { true,  {} },
            /* Stone   */ { true,  {} },
            /* Bedrock */ { true,  {} },
            /* Diorite */ { true,  {} },
            /* Granite */ { true,  {} },
            /* Sand    */ { true,  {} },
            /* Snow    */ { true,  {} },
            /* Water   */ { true,  {} },
            /* OakLog  */ { true,  {} },
            /* Leaves  */ { true,  {} },
        }
    };

    // Generate compiler error if a block is not added to the info table
    static_assert(kBlocks.size() == static_cast<std::size_t>(Block::COUNT));

    // Fetch info table
    inline const BlockInfo& GetBlockInfo(Block _block)
    {
        return kBlocks[static_cast<std::size_t>(_block)];
    }

    // Fetch info table
    inline const BlockInfo& GetBlockInfo(BlockId _block)
    {
        return kBlocks[_block];
    }

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
