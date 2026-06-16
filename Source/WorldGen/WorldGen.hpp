
#pragma once
#include <cstdint>
#include "Voxels/Chunk.h"
#include "Voxels/ChunkData.h"
#include "Noise.hpp"
#include "WorldGenConfig.h"

namespace WORLDGEN
{
    // Surface Y for world column
    inline int TerrainHeight(int _wx, int _wz, const WorldGenConfig& _config)
    {
        uInt32      seed    = _config.seed;
        const float scale   = _config.heightScale;
        const int   base    = _config.heightBase;
        const int   amp     = _config.heightAmp;
        const int   octaves = _config.heightOctaves;

        const float val = FBM(_wx / scale, _wz / scale, seed, octaves);
        return base + static_cast<int>(val * amp);
    }

    // generate column of the given chunk
    inline void GenerateColumn(RR::Chunk& _chunk, const WorldGenConfig& _config)
    {
        using namespace RR::CHUNK;
        const int ox = _chunk.coord.x * kSizeX;
        const int oz = _chunk.coord.z * kSizeZ;
        const int dirtDepth = _config.dirtDepth;

        for (int z = 0; z < kSizeZ; ++z)
        {
            for (int x = 0; x < kSizeX; ++x)
            {
                const int terrHeight = TerrainHeight(ox + x, oz + z, _config);

                for (int y = 0; y <= terrHeight && y < kSizeY; y++)
                {
                    BLOCK block;

                    // Build blocks per layer
                    if (y == 0) {
                        block = BLOCK::BEDROCK;
                    }
                    else if (y <  terrHeight - dirtDepth) {
                        block = BLOCK::STONE;
                    }
                    else if (y <  terrHeight) {
                        block = BLOCK::DIRT;
                    }
                    else {
                        block = BLOCK::GRASS;
                    }

                    _chunk.Set(x, y, z, static_cast<BlockId>(block));
                }
            }
        }
    }
}
