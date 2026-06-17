
#pragma once
#include <cstdint>
#include <algorithm>
#include "Voxels/Chunk.h"
#include "Voxels/ChunkData.h"
#include "Noise.hpp"
#include "WorldGenConfig.h"
#include "Biome.hpp"

namespace WORLDGEN
{
    // Surface height of a world column: per biome base
    inline int TerrainHeight(int _wx, int _wz, const BiomeGrid& _grid, const WorldGenConfig& _config)
    {
        const float noise = FBM(
            _wx / _config.heightScale,
            _wz / _config.heightScale,
            _config.seed,
            _config.heightOctaves);

        // blend lowland base/amp AND count mountains together
        const int radius = _config.biomeBlendRadius;
        long sumBase  = 0;
        long sumAmp   = 0;
        int  lowCount = 0;
        int mtnCount  = 0;

        for (int dz = -radius; dz <= radius; ++dz)
        {
            for (int dx = -radius; dx <= radius; ++dx)
            {
                const BIOME biome = _grid.At(_wx + dx, _wz + dz);

                if (biome == BIOME::MOUNTAINS)
                {
                    ++mtnCount;
                }
                else
                {
                    sumBase += _config.biomeBaseHeight[static_cast<int>(biome)];
                    sumAmp  += _config.biomeAmplitude [static_cast<int>(biome)];
                    ++lowCount;
                }
            }
        }


        const int   total = (2 * radius + 1) * (2 * radius + 1);
        const float frac  = static_cast<float>(mtnCount) / total;
        const float rawMask = std::clamp((frac - 0.5f) * 2.0f, 0.0f, 1.0f);
        const float mask = std::pow(rawMask, _config.mountainCurve);

        const float lowBase = lowCount ? float(sumBase) / lowCount : float(_config.biomeBaseHeight[(int)BIOME::MOUNTAINS]);
        const float lowAmp  = lowCount ? float(sumAmp)  / lowCount : float(_config.biomeAmplitude [(int)BIOME::MOUNTAINS]);

        const float lowlandHeight  = lowBase + noise * lowAmp;
        const float mountainHeight = _config.biomeBaseHeight[(int)BIOME::MOUNTAINS]
                                   + noise * _config.biomeAmplitude[(int)BIOME::MOUNTAINS];

        return static_cast<int>(lowlandHeight + (mountainHeight - lowlandHeight) * mask);
    }

    // Blocks to carve down at this column for water: a ridged river channel,
    // Returns 0 for no water.
    inline int WaterCarve(int _wx, int _wz, BIOME _biome, const WorldGenConfig& _config)
    {
        int carveDepth = 0;

        // FBM noise is used to trace the river path
        const float riverNoise = FBM(
            _wx / _config.riverScale,
            _wz / _config.riverScale,
            _config.seed + 601u,
            _config.riverNoiseOct);

        const float distFromCentre = std::abs(riverNoise - 0.5f) * 2.0f;

        if (distFromCentre < _config.riverWidth)
        {
            // blend from 0 to 1 to shape a channel
            const float bankBlend  = distFromCentre / _config.riverWidth;
            const int   riverCarve = static_cast<int>(_config.riverDepth * (1.0f - bankBlend));
            carveDepth = std::max(carveDepth, riverCarve);
        }

        // Ponds - plain taiga only
        if (_biome == BIOME::PLAINS || _biome == BIOME::TAIGA)
        {
            const float pondNoise = FBM(
                _wx / _config.pondScale,
                _wz / _config.pondScale,
                _config.seed + 602u,
                _config.pondNoiseOct);

            if (pondNoise < _config.pondThreshold)
            {
                // Blend from 0 to 1 to shape a bowl
                const float pondBlend = pondNoise / _config.pondThreshold;
                const int   pondCarve = static_cast<int>(_config.pondDepth * (1.0f - pondBlend));
                carveDepth = std::max(carveDepth, pondCarve);
            }
        }

        return carveDepth;
    }

    // Which stone/ore fills a deep block: 3D noise fields pick rare ore veins
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

    // Generate every column of a chunk: pick the biome from cellular grid
    // This is the injected ChunkGenerator callback: pure, runs once per chunk
    inline void GenerateColumn(RR::Chunk& _chunk, const WorldGenConfig& _config)
    {
        using namespace RR::CHUNK;
        const int ox = _chunk.coord.x * kSizeX;
        const int oz = _chunk.coord.z * kSizeZ;
        const int dirtDepth  = _config.dirtDepth;
        const int waterLevel = _config.waterLevel;

        const int margin = _config.biomeBlendRadius;
        const BiomeGrid grid = BuildBiomeGrid(_chunk.coord.x, _chunk.coord.z, margin, _config);

        for (int z = 0; z < kSizeZ; ++z)
        {
            for (int x = 0; x < kSizeX; ++x)
            {
                const int wx = ox + x;
                const int wz = oz + z;
                // Get Biome
                const BIOME biome = grid.At(wx, wz);
                const BiomeParams& bParams = GetBiome(biome);
                // Get Land Height
                const int land  = TerrainHeight(wx, wz, grid, _config);
                const int carve = WaterCarve(wx, wz, biome, _config);
                const int terrHeight = land - carve;

                int waterTop = waterLevel;
                if (carve > 0) {
                waterTop = std::max(waterTop, land - 1);
                }
                const bool underWater = (terrHeight < waterTop);

                // Solid column
                for (int y = 0; y <= terrHeight && y < kSizeY; ++y)
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
                        block = bParams.subsurface;
                    }
                    else {
                        block = underWater ? bParams.subsurface : bParams.surface;
                    }

                    _chunk.Set(x, y, z, static_cast<BlockId>(block));
                }

                // If liquid - ice in frozen biomes
                if (underWater)
                {
                    const bool frozen  = (biome == BIOME::TAIGA || biome == BIOME::TUNDRA);
                    const BLOCK liquid = frozen ? BLOCK::ICE : BLOCK::WATER;

                    for (int y = terrHeight + 1; y <= waterTop && y < kSizeY; ++y)
                    {
                        _chunk.Set(x,y,z, static_cast<BlockId>(liquid));
                    }
                }
            }
        }
    }
}
