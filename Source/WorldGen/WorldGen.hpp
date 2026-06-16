
#pragma once
#include <cstdint>
#include "Voxels/Chunk.h"
#include "Voxels/ChunkData.h"
#include "Noise.hpp"
#include "WorldGenConfig.h"
#include "Biome.hpp"

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

    // Returns what kind of stone in the strata is present
    inline BLOCK StoneAt(int _wx, int _wy, int _wz, const WorldGenConfig& _config)
    {
        const uInt32 seed = _config.seed;

        // Ores: rare, smaller
        const float oreSc = _config.oreScale;
        if (ValueNoise3(_wx/oreSc, _wy/oreSc, _wz/oreSc, seed + 401u) > _config.coalThresh)
            return BLOCK::COAL_ORE;

        if (ValueNoise3(_wx/oreSc, _wy/oreSc, _wz/oreSc, seed + 402u) > _config.ironThresh)
            return BLOCK::IRON_ORE;

        if (ValueNoise3(_wx/oreSc, _wy/oreSc, _wz/oreSc, seed + 403u) > _config.copperThresh)
            return BLOCK::COPPER_ORE;

        // Strata bigger, common (diorite/granite)
        const float strataSc = _config.strataScale;
        if (ValueNoise3(_wx/strataSc, _wy/strataSc, _wz/strataSc, seed + 501u) > _config.dioriteThresh)
            return BLOCK::DIORITE;

        if (ValueNoise3(_wx/strataSc, _wy/strataSc, _wz/strataSc, seed + 502u) > _config.graniteThresh)
            return BLOCK::GRANITE;

        // Default to stone
        return BLOCK::STONE;
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
                const int wx = ox + x;
                const int wz = oz + z;
                const int terrHeight = TerrainHeight(ox + x, oz + z, _config);
                // select biome for current column
                const BiomeParams& bParams = GetBiome(SelectBiome(wx, wz, _config));

                for (int y = 0; y <= terrHeight && y < kSizeY; y++)
                {
                    BLOCK block;

                    // Build blocks per layer
                    if (y == 0) {
                        block = BLOCK::BEDROCK;
                    }
                    else if (y <  terrHeight - dirtDepth) {
                        block = StoneAt(wx, y, wz, _config);
                    }
                    else if (y <  terrHeight) {
                        //block = bParams.subsurface;
                        block = BLOCK::AIR;
                    }
                    else {
                        //block = bParams.surface;
                        block = BLOCK::AIR;
                        
                    }

                    _chunk.Set(x, y, z, static_cast<BlockId>(block));
                }
            }
        }
    }


}
