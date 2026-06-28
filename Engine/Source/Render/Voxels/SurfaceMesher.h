
#pragma once
#include <vector>

#include "Voxels/ChunkData.h"
#include "Render/MeshData.h"

namespace RR
{
    // Triangulates a dim x dim strided field
    // aka meshes top faces only for lod
    MeshData MeshSurface(int _dim, int _level,
        const std::vector<int>& _height,
        const std::vector<CHUNK::BLOCK>& _block);
}
