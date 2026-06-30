
#pragma once
#include <vector>

#include "Voxels/ChunkData.h"
#include "Render/MeshData.h"

namespace RR
{
    // Visible cliff face depth
    constexpr int kLodBandDepth = 32;
    // how many steps
    constexpr int kLodBandStep  = 1;
    struct LodTreeProxy
    {
        int localX;
        int localZ;
        int baseY;
        int trunkHeight;    // visible log below the canopy
        int canopyHeight;   // foliage height
        int radius;         // crown half ehgith 

        float crownTopFrac; // crown top width

        CHUNK::BLOCK log;
        CHUNK::BLOCK canopy;
    };

    // Triangulates a dim x dim strided field
    // aka meshes top faces only for lod
    MeshData MeshSurface(int _dim, int _level,
        const std::vector<int>& _height,
        const std::vector<CHUNK::BLOCK>& _block,
        const std::vector<CHUNK::BLOCK>& _sideColumn,
        int _skirtDepth = 0);

    // Triangulates vegetation canopies
    MeshData MeshProxies(const std::vector<LodTreeProxy>& _trees, int _level);
}
