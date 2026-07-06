
#pragma once
#include <array>
#include <memory>
#include <functional>
#include <cstdint>

#include "ChunkData.h"

namespace RR
{
    class PooledMesh;
    struct Chunk
    {
        explicit Chunk(CHUNK::Coord _coord);
        ~Chunk();

        // fetch/set
        CHUNK::BlockId At (int _x, int _y, int _z) const
        {
            return voxels[CHUNK::Index(_x, _y, _z)];
        }

        void Set(int _x, int _y, int _z, CHUNK::BlockId _id)
        {
            voxels[CHUNK::Index(_x, _y, _z)] = _id;
        }

        // Position and state of the chunk
        CHUNK::Coord coord;
        CHUNK::STATE state = CHUNK::STATE::EMPTY;

        // Chunk contents and meshes
        std::array<CHUNK::BlockId, CHUNK::kVoxelsPerChunk> voxels {};
        std::unique_ptr<PooledMesh> mesh;
        std::unique_ptr<PooledMesh> vegMesh;

        // index for frustum call checks
        std::uint64_t lastVisit = 0;
    };

    using ChunkGenerator = std::function<void(Chunk&)>;
}
