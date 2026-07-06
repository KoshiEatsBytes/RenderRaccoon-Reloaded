
#pragma once
#include <memory>
#include <cstdint>

#include "ChunkData.h"

namespace RR
{
    class PooledMesh;

    // distant voxel-less surface tile, meshed as a strided surface skin
    struct LodTile
    {
        int coreEdges = 0;
        std::unique_ptr<PooledMesh> mesh;
        std::unique_ptr<PooledMesh> proxyMesh;

        // view quad-tree last visit
        std::uint64_t lastVisit = 0;
    };
}