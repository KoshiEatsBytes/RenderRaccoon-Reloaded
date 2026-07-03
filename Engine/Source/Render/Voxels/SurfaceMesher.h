
#pragma once
#include <vector>

#include "Voxels/ChunkData.h"
#include "Render/MeshData.h"

namespace RR
{
    // Visible cliff face depth
    constexpr int kLodBandDepth = 32;
    // banding settings
    static constexpr int kLodBandStepL1 = 1;
    static constexpr int kLodBandStepL2 = 2;
    static constexpr int kLodBandStepL3 = 4;

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
    MeshData MeshSurface(int _dim, int _level, int _bandMaxLevel, bool _greedy,
        const std::vector<int>& _height,
        const std::vector<CHUNK::BLOCK>& _block,
        const std::vector<CHUNK::BLOCK>& _sideColumn);

    // Triangulates vegetation canopies
    MeshData MeshProxies(const std::vector<LodTreeProxy>& _trees, int _level);
}
