
#pragma once
#include <array>
#include <memory>
#include <functional>

#include "ChunkData.h"

namespace RR
{
    class Mesh;
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
        CHUNK::State state = CHUNK::State::EMPTY;

        // Chunk contents and mesh
        std::array<CHUNK::BlockId, CHUNK::kVoxelsPerChunk> voxels {};
        std::unique_ptr<Mesh> mesh;
    };

    using ChunkGenerator = std::function<void(Chunk&)>;
}
