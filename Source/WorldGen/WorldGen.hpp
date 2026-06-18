
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
        int base  = 0;
        int amp   = 0;
        int mtn   = 0;
        int taiga = 0;
    };

    // Optimization to blend biomes more efficently
    inline std::array<BlendSums, 256> BuildBlendSums(const BiomeGrid& _grid, const WorldGenConfig& _config)
    {
        const int R   = _config.biomeBlendRadius;
        const int gw  = _grid.w;
        const int win = 2 * R + 1;

        // per grid-cell contribution
        std::vector<int> sBase (gw * gw);
        std::vector<int> sAmp  (gw * gw);
        std::vector<int> sMtn  (gw * gw);
        std::vector<int> sTaiga(gw * gw);

        for (int gj = 0; gj < gw; ++gj)
        {
            for (int gi = 0; gi < gw; ++gi)
            {
                const int idx = gi + gj * gw;
                const BIOME biome = _grid.cells[idx];
                if (biome == BIOME::MOUNTAINS)
                {
                    sMtn[idx] = 1;
                }
                else
                {
                    sBase[idx] = _config.biomeBaseHeight[static_cast<int>(biome)];
                    sAmp[idx]  = _config.biomeAmplitude[static_cast<int>(biome)];
                    if (biome == BIOME::TAIGA) sTaiga[idx] = 1;
                }
            }
        }

        // horizontal pass
        std::vector<int> hB(16 * gw), hA(16 * gw), hM(16 * gw), hT(16 * gw);
        for (int gj = 0; gj < gw; ++gj)
        {
            for (int x = 0; x < 16; ++x)
            {
                int b = 0, a = 0, m = 0, t = 0;
                for (int k = 0; k < win; ++k) { const int i = (x + k) + gj * gw; b += sBase[i]; a += sAmp[i]; m += sMtn[i]; t += sTaiga[i]; }
                const int hi = x + gj * 16; hB[hi] = b; hA[hi] = a; hM[hi] = m; hT[hi] = t;
            }
        }

        // vertical pass
        std::array<BlendSums, 256> out{};
        for (int z = 0; z < 16; ++z)
        {
            for (int x = 0; x < 16; ++x)
            {
                int b = 0, a = 0, m = 0, t = 0;
                for (int k = 0; k < win; ++k) { const int hi = x + (z + k) * 16; b += hB[hi]; a += hA[hi]; m += hM[hi]; t += hT[hi]; }
                out[x + z * 16] = { b, a, m, t };
            }
        }

        return out;
    }

    // Mountain influence at a column from the blend sums, 0 is lowland
    inline float MountainMask(const BlendSums& _s, int _total, const WorldGenConfig& _config)
    {
        const float rawMask = std::clamp((float(_s.mtn) / _total - 0.5f) * 2.0f, 0.0f, 1.0f);
        return std::pow(rawMask, _config.mountainCurve);
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

        const float mask = MountainMask(_s, _total, _config);

        const float lowlandHeight  = lowBase + noise * lowAmp;
        float mountainHeight = 0.0f;

        if (_config.ridgeMountains)
        {
            const float ridged = 1.0f - std::abs(noise * 2.0f - 1.0f);
            const float mNoise = Lerp(noise, ridged, _config.ridgeStrength);
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
    // The snow lines wobble together via a noise jitter, so the snow line is irregular
    inline BLOCK MountainSurface(int _y, int _wx, int _wz, const WorldGenConfig& _config)
    {
        const float jitter = (FBM(_wx / _config.snowJitterScale, _wz / _config.snowJitterScale,
            _config.seed + 711u, 2) - 0.5f) * 2.0f * _config.snowJitterAmp;

        if (_y >= _config.snowLine     + jitter) return BLOCK::SNOW;
        if (_y >= _config.mtnGrassLine + jitter) return BLOCK::STONE;

        return BLOCK::GRASS;
    }

    // One river field's valley profile from already warped coords
    inline float RiverFieldProfile(float _rx, float _rz, float _allow, float _scale, float _valleyWidth, uInt32 _salt, int _oct)
    {
        const float noise = FBM(_rx/_scale, _rz/_scale, _salt, _oct);
        const float dist = std::abs(noise - 0.5f) * 2.0f;

        if (dist >= _valleyWidth) return 0.0f;
        const float result = Smooth(1.0f - dist/_valleyWidth) * _allow;

        return result;
    }

    // Raw river meander profile 0 1 at a column 
    inline float RiverProfile(int _wx, int _wz, const WorldGenConfig& _config)
    {
        // warp once, both fields share the same meander
        float rx = static_cast<float>(_wx), rz = static_cast<float>(_wz);

        if (_config.riverWarpEnabled)
        {
            const float px = _wx / _config.riverWarpScale, pz = _wz / _config.riverWarpScale;
            rx += (FBM(px, pz, _config.seed + 935u, _config.warpOctaves) - 0.5f) * 2.0f * _config.riverWarpAmp;
            rz += (FBM(px, pz, _config.seed + 936u, _config.warpOctaves) - 0.5f) * 2.0f * _config.riverWarpAmp;
        }

        // trunk = wide full-depth main rivers
        float profile = RiverFieldProfile(
            rx, rz, 1.0f,
            _config.riverScale,
            _config.riverValleyWidth,
            _config.seed + 601u,
            _config.riverNoiseOct);

        // tributaries: smaller scale, narrower, shallower, merge into trunks via max
        if (_config.tributariesEnabled)
        {
            const float trib = RiverFieldProfile(rx, rz, 1.0f, _config.tribScale, _config.tribValleyWidth, _config.seed + 603u, _config.riverNoiseOct);
            profile = std::max(profile, trib * _config.tribStrength);
        }
        return profile;
    }

    // Generate every column of a chunk: pick the biome from cellular grid
    // This is the injected ChunkGenerator callback: pure, runs once per chunk
    inline void GenerateColumn(RR::Chunk& _chunk, const WorldGenConfig& _config)
    {
        using namespace RR::CHUNK;
        const int ox = _chunk.coord.x * kSizeX;
        const int oz = _chunk.coord.z * kSizeZ;

        const int dirtDepth  = _config.dirtDepth;
        const int margin     = _config.biomeBlendRadius;
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
                const BlendSums& sum = sums[x + z * 16];
                // Land height + mountain mask (mask shared with the tunnel gate)
                const int   land    = TerrainHeightFromSums(sum, wx, wz, total, _config);
                const float mtnMask = MountainMask(sum, total, _config);

                // Raw meander (for tunnels) 
                float profile = RiverProfile(wx, wz, _config);
                if (!_config.taigaRivers) profile *= 1.0f - float(sum.taiga) / total;
                const float riverAllow = std::clamp(float(_config.riverMaxHeight - land) / _config.riverFade, 0.0f, 1.0f);
                const float valleyTerr = profile * riverAllow;

                int waterTop   = _config.waterLevel;
                int terrHeight = land;
                int capBase    = -1;   

                if (profile > 0.0f)
                {
                    const int shelfBed   = _config.riverLevel - _config.riverShelfDepth;
                    const int channelBed = _config.riverLevel - _config.riverDepth;

                    // Tunnel ceiling: arched 
                    const float arch    = _config.riverArchHeight * Smooth(profile);
                    const float jit     = FBM(wx / _config.riverCeilScale, wz / _config.riverCeilScale, _config.seed + 960u, 2) * _config.riverCeilJitter;
                    const int   ceiling = _config.riverLevel + static_cast<int>(arch) - static_cast<int>(jit);

                    if (_config.riverTunnels && mtnMask > _config.tunnelMaskThresh
                        && ceiling > _config.riverLevel && land > ceiling)
                    {
                        // TUNNEL carve a channel arched void through the mountain
                        const float floorF = Lerp(static_cast<float>(_config.riverLevel), static_cast<float>(channelBed), profile);
                        terrHeight = static_cast<int>(floorF);
                        waterTop   = std::max(_config.waterLevel, _config.riverLevel);
                        capBase    = ceiling;   
                    }
                    else if (valleyTerr > 0.0f)
                    {
                        // ramp terrain down to the channel, flat water fills it.
                        float bed = Lerp(static_cast<float>(land), static_cast<float>(shelfBed), valleyTerr);
                        const float channelT = std::clamp((valleyTerr - _config.channelThreshold) / (1.0f - _config.channelThreshold), 0.0f, 1.0f);
                        bed = Lerp(bed, static_cast<float>(channelBed), channelT);
                        terrHeight = std::min(land, static_cast<int>(bed));
                        waterTop   = std::max(_config.waterLevel, _config.riverLevel);
                    }
                }
                const bool underWater = terrHeight < waterTop;

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
                        const float sandRoll = HashFloat(wx, wz, _config.seed + 950u);

                        if (underWater)
                        {
                            // river bed, scattered sand floor instead of plain dirt
                            block = (sandRoll < _config.beachSandChance) ? BLOCK::SAND : bParams.subsurface;
                        }
                        else if (biome == BIOME::MOUNTAINS)
                        {
                            block = MountainSurface(terrHeight, wx, wz, _config);
                        }
                        else
                        {
                            // sand around water hedge
                            const int shelfBed = _config.riverLevel - _config.riverShelfDepth;
                            float     waterT   = 1.0f;

                            if (land > shelfBed)
                            {
                                waterT = static_cast<float>(land - waterTop) / static_cast<float>(land - shelfBed);
                            }

                            // desert uses grass instead of sand for river
                            const bool  arid       = (biome == BIOME::DESERT || biome == BIOME::RED_DESERT);
                            const BLOCK beachBlock = (arid && _config.desertRiverGrass) ? BLOCK::GRASS : BLOCK::SAND;

                            if (valleyTerr > 0.0f && waterT - valleyTerr <= _config.beachBand)
                            {
                                block = (sandRoll < _config.beachSandChance) ? beachBlock : bParams.surface;
                            }
                            else
                            {
                                block = bParams.surface;
                            }
                        }

                    }

                    _chunk.Set(x, y, z, static_cast<BlockId>(block));
                }

                // If liquid - ice in frozen biomes
                if (underWater)
                {
                    const bool frozen  = _config.iceEnabled && (biome == BIOME::TAIGA || biome == BIOME::TUNDRA);
                    const BLOCK liquid = frozen ? BLOCK::ICE : BLOCK::WATER;

                    for (int y = terrHeight + 1; y <= waterTop && y < kSizeY; ++y)
                    {
                        _chunk.Set(x,y,z, static_cast<BlockId>(liquid));
                    }
                }

                // River-tunnel cap: solid mountain resumes above the arched void
                if (capBase >= 0)
                {
                    for (int y = capBase + 1; y <= land && y < kSizeY; ++y)
                    {
                        BLOCK block = (y == land) ? MountainSurface(land, wx, wz, _config)
                                                  : StoneAt(wx, y, wz, _config);

                        // Dripstone/calcite formations on the tunnel ceiling & upper walls 
                        if (y < land && y <= capBase + _config.calciteBand
                            && HashFloat3(wx, y, wz, _config.seed + 970u) < _config.calciteChance)
                        {
                            // mostly dripstone, calcite accents
                            block = (HashFloat3(wx, y, wz, _config.seed + 971u) < _config.dripstoneFraction)
                                    ? BLOCK::DRIPSTONE : BLOCK::CALCITE;
                        }
                        _chunk.Set(x, y, z, static_cast<BlockId>(block));
                    }
                }
            }
        }
    }
}
