
#pragma once
#include <array>

#include "Voxels/Chunk.h"
#include "Render/MeshData.h"

namespace RR
{
    // check all 4 neighbors for culling
    // air by default unless populated
    struct ChunkBorders
    {
        enum class Border
        {
            WEST,
            EAST,
            NORTH,
            SOUTH
        };

        // Run the 2D index on the single dim arrays
        static constexpr int Index(int _lateral, int _y)
        {
            return _lateral * CHUNK::kSizeY + _y;
        }

        CHUNK::BlockId GetBorderVoxel(Border _border, int _x, int _y, int _z) const
        {
            switch (_border)
            {
                case Border::EAST:
                    return m_east[Index(_z, _y)];

                case Border::WEST:
                    return m_west[Index(_z, _y)];

                case Border::SOUTH:
                    return m_south[Index(_x, _y)];

                case Border::NORTH:
                    return m_north[Index(_x, _y)];

                default:
                    return static_cast<CHUNK::BlockId>(CHUNK::BLOCK::AIR);
            }
        }

        void SetBorderVoxel(Border _border, CHUNK::BlockId _id, int _x, int _y, int _z)
        {
            switch (_border)
            {
                case Border::EAST:
                    m_east[Index(_z, _y)] = _id;
                    return;

                case Border::WEST:
                    m_west[Index(_z, _y)] = _id;
                    return;

                case Border::SOUTH:
                    m_south[Index(_x, _y)] = _id;
                    return;

                case Border::NORTH:
                    m_north[Index(_x, _y)] = _id;
                    return;
            }
        }

    private:
        std::array<CHUNK::BlockId, CHUNK::kSizeZ * CHUNK::kSizeY> m_east {};
        std::array<CHUNK::BlockId, CHUNK::kSizeZ * CHUNK::kSizeY> m_west {};
        std::array<CHUNK::BlockId, CHUNK::kSizeX * CHUNK::kSizeY> m_south{};
        std::array<CHUNK::BlockId, CHUNK::kSizeX * CHUNK::kSizeY> m_north{};
    };

    // Voxel vertex attribute locations
    namespace VoxelAttrib
    {
        inline constexpr GLuint Position = 0;
        inline constexpr GLuint Normal   = 1;
        inline constexpr GLuint UV       = 2;
        inline constexpr GLuint Layer    = 3;
    }

    VertexLayout VoxelVertexLayout();

    // Pure mesher, automatically culls hiding faces
    MeshData MeshChunk(const Chunk& _chunk, const ChunkBorders& _borders);
}














