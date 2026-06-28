
#pragma once
#include <memory>

#include "ChunkData.h"

namespace RR
{
    class Mesh;

    // distant voxel-less surface tile, meshed as a strided surface skin
    struct LodTile
    {
        CHUNK::Coord cords;
        int level = 1;

        std::unique_ptr<Mesh> mesh;
    };
}