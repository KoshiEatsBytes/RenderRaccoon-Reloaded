
#pragma once
#include <memory>

#include "ChunkData.h"

namespace RR
{
    class Mesh;

    // distant voxel-less surface tile, meshed as a strided surface skin
    struct LodTile
    {
        int coreEdges = 0;
        std::unique_ptr<Mesh> mesh;
        std::unique_ptr<Mesh> proxyMesh;
    };
}