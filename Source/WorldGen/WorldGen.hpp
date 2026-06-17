
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
    struct BlendSums
    {
        int base = 0;
        int amp = 0;
        int mtn = 0;
    };

    // Surface height of a world column: per biome base
    // DEPRECATED
    // inline int TerrainHeight(int _wx, int _wz, const BiomeGrid& _grid, const WorldGenConfig& _config)
    // {
    //     const float noise = FBM(
    //         _wx / _config.heightScale,
    //         _wz / _config.heightScale,
    //         _config.seed,
    //         _config.heightOctaves);
    //
    //     // blend lowland base/amp AND count mountains together
    //     const int radius = _config.biomeBlendRadius;
    //     long sumBase  = 0;
    //     long sumAmp   = 0;
    //     int  lowCount = 0;
    //     int mtnCount  = 0;
    //
    //     for (int dz = -radius; dz <= radius; ++dz)
    //     {
    //         for (int dx = -radius; dx <= radius; ++dx)
    //         {
    //             const BIOME biome = _grid.At(_wx + dx, _wz + dz);
    //
    //             if (biome == BIOME::MOUNTAINS)
    //             {
    //                 ++mtnCount;
    //             }
    //             else
    //             {
    //                 sumBase += _config.biomeBaseHeight[static_cast<int>(biome)];
    //                 sumAmp  += _config.biomeAmplitude [static_cast<int>(biome)];
    //                 ++lowCount;
    //             }
    //         }
    //     }
    //
    //
    //     const int   total = (2 * radius + 1) * (2 * radius + 1);
    //     const float frac  = static_cast<float>(mtnCount) / total;
    //     const float rawMask = std::clamp((frac - 0.5f) * 2.0f, 0.0f, 1.0f);
    //     const float mask = std::pow(rawMask, _config.mountainCurve);
    //
    //     const float lowBase = lowCount ? float(sumBase) / lowCount : float(_config.biomeBaseHeight[(int)BIOME::MOUNTAINS]);
    //     const float lowAmp  = lowCount ? float(sumAmp)  / lowCount : float(_config.biomeAmplitude [(int)BIOME::MOUNTAINS]);
    //
    //     const float lowlandHeight  = lowBase + noise * lowAmp;
    //     const float mountainHeight = _config.biomeBaseHeight[(int)BIOME::MOUNTAINS]
    //                                + noise * _config.biomeAmplitude[(int)BIOME::MOUNTAINS];
    //
    //     return static_cast<int>(lowlandHeight + (mountainHeight - lowlandHeight) * mask);
    // }

    // Optimization to blend biomes more efficently
    inline std::array<BlendSums, 256> BuildBlendSums(const BiomeGrid& _grid, const WorldGenConfig& _config)
    {
        const int R   = _config.biomeBlendRadius;
        const int gw  = _grid.w;
        const int win = 2 * R + 1;

        // per grid-cell contribution
        std::vector<int> sBase(gw * gw), sAmp(gw * gw), sMtn(gw * gw);
        for (int gj = 0; gj < gw; ++gj)
        {
            for (int gi = 0; gi < gw; ++gi)
            {
                const int idx = gi + gj * gw;
                const BIOME b = _grid.cells[idx];
                if (b == BIOME::MOUNTAINS) sMtn[idx] = 1;
                else { sBase[idx] = _config.biomeBaseHeight[(int)b]; sAmp[idx] = _config.biomeAmplitude[(int)b]; }
            }
        }

        // horizontal pass
        std::vector<int> hB(16 * gw), hA(16 * gw), hM(16 * gw);
        for (int gj = 0; gj < gw; ++gj)
        {
            for (int x = 0; x < 16; ++x)
            {
                int b = 0, a = 0, m = 0;
                for (int k = 0; k < win; ++k) { const int i = (x + k) + gj * gw; b += sBase[i]; a += sAmp[i]; m += sMtn[i]; }
                const int hi = x + gj * 16; hB[hi] = b; hA[hi] = a; hM[hi] = m;
            }
        }

        // vertical pass
        std::array<BlendSums, 256> out{};
        for (int z = 0; z < 16; ++z)
        {
            for (int x = 0; x < 16; ++x)
            {
                int b = 0, a = 0, m = 0;
                for (int k = 0; k < win; ++k) { const int hi = x + (z + k) * 16; b += hB[hi]; a += hA[hi]; m += hM[hi]; }
                out[x + z * 16] = { b, a, m };
            }
        }

        return out;
    }

    // Per-column height from the precomputed blend sums
    // Same math as TerrainHeight, just reading box-sums instead of scanning the window
    inline int TerrainHeightFromSums(const BlendSums& _s, int _wx, int _wz, int _total, const WorldGenConfig& _config)
    {
        float sx = static_cast<float>(_wx);
        float sz = static_cast<float>(_wz);
        if (_config.warpEnabled)
        {
            const float px = _wx / _config.warpScale;
            const float pz = _wz / _config.warpScale;

            // level 1 displacement
            const float d1x = (FBM(px, pz, _config.seed + 931u, _config.warpOctaves) - 0.5f) * 2.0f * _config.warpAmp;
            const float d1z = (FBM(px, pz, _config.seed + 932u, _config.warpOctaves) - 0.5f) * 2.0f * _config.warpAmp;

            if (_config.warpLevels >= 2)
            {
                // sample the warp AGAIN
                const float qx = (_wx + d1x) / _config.warpScale;
                const float qz = (_wz + d1z) / _config.warpScale;
                sx += (FBM(qx, qz, _config.seed + 933u, _config.warpOctaves) - 0.5f) * 2.0f * _config.warpAmp;
                sz += (FBM(qx, qz, _config.seed + 934u, _config.warpOctaves) - 0.5f) * 2.0f * _config.warpAmp;
            }
            else
            {
                sx += d1x;
                sz += d1z;
            }
        }
        const float noise = FBM(
            sx / _config.heightScale,
            sz / _config.heightScale,
            _config.seed,
            _config.heightOctaves,
            _config.useGradientNoise);

        const int mB = _config.biomeBaseHeight[(int)BIOME::MOUNTAINS];
        const int mA = _config.biomeAmplitude [(int)BIOME::MOUNTAINS];

        const int   lowCount = _total - _s.mtn;
        const float lowBase  = lowCount ? float(_s.base) / lowCount : float(mB);
        const float lowAmp   = lowCount ? float(_s.amp)  / lowCount : float(mA);

        const float rawMask = std::clamp((float(_s.mtn) / _total - 0.5f) * 2.0f, 0.0f, 1.0f);
        const float mask    = std::pow(rawMask, _config.mountainCurve);

        const float lowlandHeight  = lowBase + noise * lowAmp;
        float mountainHeight = 0.0f;

        if (_config.ridgeMountains)
        {
            const float ridged = 1.0f - std::abs(noise * 2.0f - 1.0f);
            const float mNoise = Lerp(noise, ridged, _config.ridgeAmp);
            mountainHeight = mB + mNoise * mA;
        }
        else
        {
            mountainHeight = mB + noise * mA;
        }

        float height = lowlandHeight + (mountainHeight - lowlandHeight) * mask;
        if (_config.detailEnabled)
        {
            height += (FBM(_wx / _config.detailScale, _wz / _config.detailScale,
                      _config.seed + 941u, _config.detailOctaves) - 0.5f) * 2.0f * _config.detailAmp;
        }
        return static_cast<int>(height);
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

    // Mountain surface by elevation
    // The snow/grass lines wobble together via a noise jitter, so the snow line is irregular
    inline BLOCK MountainSurface(int _y, int _wx, int _wz, const WorldGenConfig& _config)
    {
        const float jitter = (FBM(_wx / _config.snowJitterScale, _wz / _config.snowJitterScale,
            _config.seed + 711u, 2) - 0.5f) * 2.0f * _config.snowJitterAmp;

        if (_y >= _config.snowLine     + jitter) return BLOCK::SNOW;
        if (_y >= _config.mtnGrassLine + jitter) return BLOCK::STONE;

        return BLOCK::GRASS;
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

        // Separable blend: precompute every column's window sums once per chunk
        const std::array<BlendSums, 256> sums = BuildBlendSums(grid, _config);
        const int total = (2 * margin + 1) * (2 * margin + 1);

        for (int z = 0; z < kSizeZ; ++z)
        {
            for (int x = 0; x < kSizeX; ++x)
            {
                const int wx = ox + x;
                const int wz = oz + z;
                // Get Biome
                const BIOME biome = grid.At(wx, wz);
                const BiomeParams& bParams = GetBiome(biome);
                // Get Land Height (separable blend; TerrainHeight kept as the oracle for testing)
                const int land  = TerrainHeightFromSums(sums[x + z * 16], wx, wz, total, _config);
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
                    if (y == 0)
                    {
                        block = BLOCK::BEDROCK;
                    }
                    else if (y <  terrHeight - dirtDepth)
                    {
                        block = StoneAt(wx, y, wz, _config);
                    }
                    else if (y <  terrHeight)
                    {
                        // Dont override mountains strata
                        block = bParams.cliffEligible ? StoneAt(wx, y, wz, _config)
                                                      : bParams.subsurface;
                    }
                    // Surface block, changes on biome
                    else
                    {
                        if (underWater) {
                            block = bParams.subsurface;
                        }
                        else if (biome == BIOME::MOUNTAINS) {
                            block = MountainSurface(terrHeight, wx, wz, _config);
                        }
                        else {
                            block = bParams.surface;
                        }
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
