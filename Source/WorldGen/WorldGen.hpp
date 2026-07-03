
#pragma once
#include <cstdint>
#include <algorithm>
#include "Voxels/Chunk.h"
#include "Voxels/ChunkData.h"
#include "Noise.hpp"
#include "WorldGenConfig.h"
#include "Biome.hpp"
#include "Vegetation.hpp"
#include "BiomeMap.hpp"

namespace WORLDGEN
{
    struct BlendSums
    {
        int base  = 0;
        int amp   = 0;
        int mtn   = 0;
        int taiga = 0;
        int mesa  = 0;
    };

    struct RiverCarve
    {
        int terrHeight = 0;
        int waterTop   = 0;
    };

    // per cell blend contribution - for full res and strided paths
    inline BlendSums SeedCell(BIOME _biome, const WorldGenConfig& _config)
    {
        BlendSums sum;
        if (_biome == BIOME::MOUNTAINS)
        {
            sum.mtn = 1;
        }
        else if (_biome == BIOME::MESA)
        {
            sum.mesa = 1;

            if (!_config.mesaRimCliffs)
            {
                sum.base = _config.biomeBaseHeight[static_cast<int>(BIOME::MESA)];
                sum.amp  = _config.biomeAmplitude [static_cast<int>(BIOME::MESA)];
            }
        }
        else
        {
            sum.base = _config.biomeBaseHeight[static_cast<int>(_biome)];
            sum.amp  = _config.biomeAmplitude [static_cast<int>(_biome)];

            if (_biome == BIOME::TAIGA) sum.taiga = 1;
        }

        return sum;
    }

    inline std::vector<BlendSums> BuildBlendSumsSpan(const BiomeGrid& _grid, int _span, const WorldGenConfig& _config)
    {
        const int radius = _config.biomeBlendRadius;
        const int gw     = _grid.w;
        const int win    = 2 * radius + 1;

        // per grid-cell contribution
        std::vector<int> sBase (gw * gw);
        std::vector<int> sAmp  (gw * gw);
        std::vector<int> sMtn  (gw * gw);
        std::vector<int> sTaiga(gw * gw);
        std::vector<int> sMesa (gw * gw);

        for (int gj = 0; gj < gw; ++gj)
        {
            for (int gi = 0; gi < gw; ++gi)
            {
                const int idx = gi + gj * gw;
                const BlendSums seed = SeedCell(_grid.cells[idx], _config);

                sBase [idx] = seed.base;
                sAmp  [idx] = seed.amp;
                sMtn  [idx] = seed.mtn;
                sTaiga[idx] = seed.taiga;
                sMesa [idx] = seed.mesa;
            }
        }

        // horizontal pass span wide
        std::vector<int> hBase (_span * gw);
        std::vector<int> hAmp  (_span * gw);
        std::vector<int> hMtn  (_span * gw);
        std::vector<int> hTaiga(_span * gw);
        std::vector<int> hMesa (_span * gw);

        for (int gj = 0; gj < gw; ++gj)
        {
            for (int x = 0; x < _span; ++x)
            {
                int base  = 0;
                int amp   = 0;
                int mtn   = 0;
                int taiga = 0;
                int mesa  = 0;

                for (int k = 0; k < win; ++k)
                {
                    const int i = (x + k) + gj * gw;
                    base  += sBase[i];
                    amp   += sAmp[i];
                    mtn   += sMtn[i];
                    taiga += sTaiga[i];
                    mesa  += sMesa[i];
                }

                const int hi = x + gj * _span;

                hBase[hi]  = base;
                hAmp[hi]   = amp;
                hMtn[hi]   = mtn;
                hTaiga[hi] = taiga;
                hMesa[hi]  = mesa;
            }
        }

        // vertical pass - span x span
        std::vector<BlendSums> out(static_cast<size_t>(_span) * _span);
        for (int z = 0; z < _span; ++z)
        {
            for (int x = 0; x < _span; ++x)
            {
                int base  = 0;
                int amp   = 0;
                int mtn   = 0;
                int taiga = 0;
                int mesa  = 0;

                for (int k = 0; k < win; ++k)
                {
                    const int hi = x + (z + k) * _span;

                    base  += hBase[hi];
                    amp   += hAmp[hi];
                    mtn   += hMtn[hi];
                    taiga += hTaiga[hi];
                    mesa  += hMesa[hi];
                }

                out[x + z * _span] = { base, amp, mtn, taiga, mesa };
            }
        }

        return out;
    }

    // Optimization to blend biomes more efficently
    inline std::vector<BlendSums> BuildBlendSums(const BiomeGrid& _grid, const WorldGenConfig& _config)
    {
        return BuildBlendSumsSpan(_grid, RR::CHUNK::kSizeX, _config);
    }

    // Mountain influence at a column from the blend sums, 0 is lowland
    inline float MountainMask(const BlendSums& _s, int _total, const WorldGenConfig& _config)
    {
        const float rawMask = std::clamp((static_cast<float>(_s.mtn) / _total - 0.5f) * 2.0f, 0.0f, 1.0f);
        return std::pow(rawMask, _config.mountainCurve);
    }

    inline float MesaMask(const BlendSums& _sums, int _total, const WorldGenConfig& _config)
    {
        return std::clamp((static_cast<float>(_sums.mesa) / _total - 0.5f) * 2.f, 0.0f, 1.0f);
    }

    // apron creating a slight skirt before biome height
    inline float ApronMask(float _mask, bool _enabled, float _thresh)
    {
        if (!_enabled) return _mask;

        return std::clamp((_mask - _thresh) / (1.0f - _thresh), 0.0f, 1.0f);
    }

    inline float SoftTerrace(float _height, float _step, float _riser)
    {
        const float level = _height / _step;
        const float floor = std::floor(level);
        const float t     = std::clamp((level - floor - (1.0f - _riser)) / _riser, 0.0f, 1.0f);

        return (floor + Smooth(t)) * _step;
    }

    inline float Cliffiness(int _wx, int _wz, float _mesaMask, const WorldGenConfig& _config)
    {
        if (!_config.cliffsEnabled || _mesaMask <= 0.0f) return 0.0f;

        const float noise = FBM(
            _wx / _config.cliffScale,
            _wz / _config.cliffScale,
            _config.seed + 980u,
            _config.cliffOctaves);

        const float clamp = std::clamp((noise - _config.cliffThreshold) / _config.cliffBlendWidth, 0.0f, 1.0f);
        return _mesaMask * Smooth(clamp);
    }

    // Per-column height from the precomputed blend sums
    // Same math as TerrainHeight, just reading box-sums instead of scanning the window
    inline int TerrainHeightFromSums(const BlendSums& _sum, int _wx, int _wz, int _total, const WorldGenConfig& _config)
    {
        auto sx = static_cast<float>(_wx);
        auto sz = static_cast<float>(_wz);

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

        const int mtnBase = _config.biomeBaseHeight[static_cast<int>(BIOME::MOUNTAINS)];
        const int mtnAmp  = _config.biomeAmplitude [static_cast<int>(BIOME::MOUNTAINS)];

        const int   sumMesa  = _config.mesaRimCliffs ? _sum.mesa : 0;
        const int   lowCount = _total - _sum.mtn - sumMesa;
        // hardcoded heights for a rare mesa on mountains collision
        const float lowBase  = lowCount ? static_cast<float>(_sum.base) / lowCount : 64.0f;
        const float lowAmp   = lowCount ? static_cast<float>(_sum.amp)  / lowCount : 8.0f;

        const float mtnMask  = MountainMask(_sum, _total, _config);
        const float mesaMask = MesaMask(_sum, _total, _config);
        const float mtnCliff  = ApronMask(mtnMask,  _config.mtnApron,  _config.mtnApronThresh);
        const float mesaCliff = ApronMask(mesaMask, _config.mesaApron, _config.mesaApronThresh);

        const float lowlandHeight  = lowBase + noise * lowAmp;
        float mountainHeight = 0.0f;

        // specific mesa noise
        const float mesaNoise  = FBM(
            sx / _config.mesaNoiseScale,
            sz / _config.mesaNoiseScale,
            _config.seed + 953u,
            _config.mesaNoiseOctaves,
            _config.useGradientNoise);

        const float mesaHeight = _config.biomeBaseHeight[static_cast<int>(BIOME::MESA)]
                                + mesaNoise * _config.biomeAmplitude[static_cast<int>(BIOME::MESA)];

        // Behaves differently if mountains have ridges
        if (_config.ridgeMountains)
        {
            const float ridged = 1.0f - std::abs(noise * 2.0f - 1.0f);
            const float mNoise = Lerp(noise, ridged, _config.ridgeStrength);
            mountainHeight = mtnBase + mNoise * mtnAmp ;
        }
        else
        {
            mountainHeight = mtnBase + noise * mtnAmp ;
        }

        float height = lowlandHeight + (mountainHeight - lowlandHeight) * mtnCliff;

        if (_config.mesaRimCliffs) {
            height += (mesaHeight - lowlandHeight) * mesaCliff;
        }

        const float cliff = Cliffiness(_wx, _wz, mesaCliff, _config);
        if (cliff > 0.0f)
        {
            const float phase = FBM(
                _wx / _config.cliffPhaseScale,
                _wz / _config.cliffPhaseScale,
                _config.seed + 981u, 2)
                * _config.cliffStep;

            const float terraced = SoftTerrace(height - phase, _config.cliffStep, _config.cliffRiser) + phase;

            height = Lerp(height, terraced, cliff);
        }

        if (_config.detailEnabled)
        {
            height += (FBM(_wx / _config.detailScale, _wz / _config.detailScale,
                      _config.seed + 941u, _config.detailOctaves) - 0.5f) * 2.0f * _config.detailAmp;
        }
        return static_cast<int>(height);
    }

    // Blend window at precomputer area from x and z
    inline int LandHeightAt(const std::vector<BIOME>& _area, int _areaOriginX, int _areaOriginZ, int areaWidth,
                            int _wx, int _wz, const WorldGenConfig& _config, BlendSums* _outSums = nullptr)
    {
        const int blendRadius = _config.biomeBlendRadius;
        BlendSums sum{};

        // Copy of blendSums once again
        for (int dz = -blendRadius; dz <= blendRadius; ++dz)
        {
            for (int dx = -blendRadius; dx <= blendRadius; ++dx)
            {
                const BIOME biome = _area[(_wx + dx - _areaOriginX) + (_wz + dz - _areaOriginZ) * areaWidth];

                if (biome == BIOME::MOUNTAINS)
                {
                    sum.mtn += 1;
                }
                else if (biome == BIOME::MESA)
                {
                    sum.mesa += 1;
                    if (!_config.mesaRimCliffs)
                    {
                        sum.base += _config.biomeBaseHeight[static_cast<int>(BIOME::MESA)];
                        sum.amp  += _config.biomeAmplitude [static_cast<int>(BIOME::MESA)];
                    }
                }
                else
                {
                    sum.base += _config.biomeBaseHeight[static_cast<int>(biome)];
                    sum.amp  += _config.biomeAmplitude [static_cast<int>(biome)];
                    if (biome == BIOME::TAIGA) sum.taiga += 1;
                }
            }
        }


        if (_outSums) *_outSums = sum;

        const int width = 2 * blendRadius + 1;
        
        return TerrainHeightFromSums(sum, _wx, _wz, width * width, _config);
    }

    // Terracotta by elevation
    inline BLOCK MesaStrata(int _y, int _wx, int _wz, const WorldGenConfig& _config)
    {
        const float jitter = (FBM(_wx / _config.mesaBandJitterScale, _wz / _config.mesaBandJitterScale,
                           _config.seed + 990u, 2) - 0.5f) * 2.0f * _config.mesaBandJitterAmp;

        const int band = static_cast<int>((static_cast<float>(_y) + jitter) / _config.mesaBandThickness);
        constexpr int num = std::size(kMesaPalette);

        return kMesaPalette[(band % num + num) % num];
    }

    // Which stone/ore fills a deep block
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

    // Raw river profile at column, low-freq noise measyred in blocks via gradient normalization, 
    // Should stop most loops, and ugly X crossings, hopefully
    inline float RiverProfile(int _wx, int _wz, const WorldGenConfig& _config)
    {
        float rangeX = static_cast<float>(_wx), rangeZ = static_cast<float>(_wz);

        if (_config.riverWarpEnabled)
        {
            const float px = _wx / _config.riverWarpScale, pz = _wz / _config.riverWarpScale;
            rangeX += (FBM(px, pz, _config.seed + 935u, _config.warpOctaves) - 0.5f) * 2.0f * _config.riverWarpAmp;
            rangeZ += (FBM(px, pz, _config.seed + 936u, _config.warpOctaves) - 0.5f) * 2.0f * _config.riverWarpAmp;
        }

        const float  scale = _config.riverScale;
        const uInt32 salt   = _config.seed + 601u;
        const int    oct    = _config.riverNoiseOct;

        auto noiseAt = [&](float x, float z) {
            return FBM(x / scale, z / scale, salt, oct);
        };

        // central difference gradient of the field
        const float noise    = noiseAt(rangeX, rangeZ);
        const float noiseX   = (noiseAt(rangeX + 1.0f, rangeZ) - noiseAt(rangeX - 1.0f, rangeZ)) * 0.5f;
        const float noiseZ   = (noiseAt(rangeX, rangeZ + 1.0f) - noiseAt(rangeX, rangeZ - 1.0f)) * 0.5f;
        const float grad     = std::sqrt(noiseX * noiseX + noiseZ * noiseZ);

        // If ring stop river creation, currently not utilez
        if (grad * scale < _config.riverGradMin) return 0.0f;

        // epsilon floor guards 0/0 
        const float distBlocks = std::abs(noise - 0.5f) / std::max(grad, 1e-6f);

        if (distBlocks >= _config.riverHalfWidth) return 0.0f;

        // Centre 1 bank 0
        return Smooth(1.0f - distBlocks / _config.riverHalfWidth);   
    }

    inline RiverCarve OpenRiverCarve(int _land, float _valleyTerr, bool _boring,
                                     BIOME _biome, const WorldGenConfig& _config)
    {
        RiverCarve out { _land, _config.waterLevel};

        if (_valleyTerr <= 0.0f) return out;

        const int shelfBed   = _config.riverLevel - _config.riverShelfDepth;
        const int channelBed = _config.riverLevel - _config.riverDepth;

        const float sharp = _biome == BIOME::MESA ? _config.riverBankSharpnessMesa
                                                  : _config.riverBankSharpnessMtn;

        float valleyTerr = _valleyTerr;
        if (_boring)
        {
            valleyTerr = std::pow(_valleyTerr, sharp);
        }

        float bed = Lerp(static_cast<float>(_land), static_cast<float>(shelfBed), valleyTerr);
        const float channelStep = std::clamp((_valleyTerr - _config.channelThreshold) /
                                             (1.0f - _config.channelThreshold), 0.0f, 1.0f);

        bed = Lerp(bed, static_cast<float>(channelBed), channelStep);

        out.terrHeight = std::min(_land, static_cast<int>(bed));
        out.waterTop   = std::max(_config.waterLevel, _config.riverLevel);
        return out;

    }

    inline void PlaceTrees(RR::Chunk& _chunk, const WorldGenConfig& _config)
    {
        using namespace RR::CHUNK;

        constexpr int FOOT = 1;
        const int areaRadius = FOOT + _config.biomeBlendRadius;
        const int areaWidth  = 2 * areaRadius + 1;

        const int outX  = _chunk.coord.x * kSizeX;
        const int outZ  = _chunk.coord.z * kSizeZ;
        const int gridW = kSizeX + 2 * kTreeMargin;

        // one biome for the margin
        const std::vector<BIOME> biomes = FinalArea(outX - kTreeMargin, outZ - kTreeMargin, gridW, gridW, _config);

        for (int lz = -kTreeMargin; lz < kSizeZ + kTreeMargin; ++lz)
        {
            for (int lx = -kTreeMargin; lx < kSizeX + kTreeMargin; ++lx)
            {
                const BIOME biome = biomes[(lx + kTreeMargin) + (lz + kTreeMargin) * gridW];
                const BiomeVegTypes& vegTypes = GetVegTypes(biome);
                const BiomeVeg&      details  = _config.biomeVegetation[static_cast<int>(biome)];

                const int wx = outX + lx;
                const int wz = outZ + lz;

                // check if tree or boulder should be placed
                // clumping modulates the per-biome tree density into thickets/clearings
                const bool placeTree    = TreeSpawnRoll(wx, wz, biome, _config);
                const bool placeBoulder = !placeTree && vegTypes.boulders &&
                                           HashFloat(wx, wz, _config.seed + 1012u) < details.boulder;

                // Discard if none
                if (!placeTree && !placeBoulder) continue;

                // CONFIRMED FEATURE SPAWN
                const int areaOriginX = wx - areaRadius;
                const int areaOriginZ = wz - areaRadius;

                const std::vector<BIOME> area = FinalArea(areaOriginX, areaOriginZ, areaWidth,
                                                        areaWidth, _config);

                BlendSums rootSums;
                const int land = LandHeightAt(area, areaOriginX, areaOriginZ,
                                              areaWidth, wx, wz, _config, &rootSums);

                // fix so trees dont avoid phantom rivers
                float profile = RiverProfile(wx, wz, _config);
                if (!_config.taigaRivers)
                {
                    const int blendTotal = (2 * _config.biomeBlendRadius + 1) * (2 * _config.biomeBlendRadius + 1);
                    profile *= 1.0f - static_cast<float>(rootSums.taiga) / blendTotal;
                }

                const float riverAllow = std::clamp(static_cast<float>(_config.riverMaxHeight - land)
                                                    / _config.riverFade, 0.f, 1.f);

                // No trees in river valleys or below sea
                if (profile * riverAllow > 0.0f) continue;
                if (land < _config.waterLevel)   continue;

                // slope spread from the same area
                int lowHeight  = land;
                int highHeight = land;

                for (int dz = -FOOT; dz <= FOOT; dz += FOOT)
                {
                     for (int dx = -FOOT; dx <= FOOT; dx += FOOT)
                    {
                        if (dx == 0 && dz == 0) continue;

                        const int height = LandHeightAt(area, areaOriginX, areaOriginZ, areaWidth,
                                                        wx + dx, wz + dz, _config);

                        lowHeight = std::min(lowHeight, height);
                        highHeight = std::max(highHeight, height);
                    }
                }

                // too steep, cant spawn, discard
                if (highHeight - lowHeight > _config.treeSlopeMax) continue;

                // dont generate boulders on tree trunks
                if (placeBoulder)
                {
                    bool nearTree = false;
                    for (int oz = -3; oz <= 3 && !nearTree; ++oz)
                    {
                        for (int ox = -3; ox <= 3; ++ox)
                        {
                            const BIOME nearBiome = area[(wx + ox - areaOriginX) + (wz + oz - areaOriginZ) * areaWidth];

                            // Discard if biome has no type of tree
                            if (GetVegTypes(nearBiome).tree == TREE::NONE) continue;

                            if (HashFloat(wx + ox, wz + oz, _config.seed + 1010u)
                                < _config.biomeVegetation[static_cast<int>(nearBiome)].tree
                                    * TreeClump(wx + ox, wz + oz, nearBiome, _config))
                            {
                                nearTree = true;
                                break;
                            }
                        }
                    }
                    if (nearTree) continue;
                }

                // Valid spawn point, hash and pick for biome
                const uInt32 shape = HashU32(wx, wz, _config.seed + 1011u);
                if (placeTree)
                {
                    switch (vegTypes.tree)
                    {
                        case TREE::OAK:
                        {
                            StampOak (_chunk, lx, land, lz, shape, details.treeMinHeight, details.treeMaxHeight);
                            break;
                        }
                        case TREE::SPRUCE_TALL:
                        {
                            StampSpruceTall(_chunk, lx, land, lz, shape, details.treeMinHeight, details.treeMaxHeight);
                            break;
                        }
                        case TREE::SPRUCE_SMALL:
                        {
                            StampSpruceSmall(_chunk, lx, land, lz, shape, details.treeMinHeight, details.treeMaxHeight);
                            break;
                        }
                        case TREE::ACACIA:
                        {
                            StampAcacia(_chunk, lx, land, lz, shape, details.treeMinHeight, details.treeMaxHeight);
                            break;
                        }
                        default:
                            break;
                    }
                }
                else StampBoulder(_chunk, lx, land, lz, shape);
            }
        }
    }

    // Generate every column of a chunk: pick the biome from cellular grid
    // This is the injected ChunkGenerator callback, runs once per chunk
    inline void GenerateColumn(RR::Chunk& _chunk, const WorldGenConfig& _config)
    {
        using namespace RR::CHUNK;
        const int ox = _chunk.coord.x * kSizeX;
        const int oz = _chunk.coord.z * kSizeZ;

        const int dirtDepth  = _config.dirtDepth;
        const int margin     = _config.biomeBlendRadius;
        const BiomeGrid grid = BuildBiomeGrid(_chunk.coord.x, _chunk.coord.z, margin, _config);

        // precompute every column's window sums once per chunk
        const std::vector<BlendSums> sums = BuildBlendSums(grid, _config);
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
                // Land height + mountain mask 
                const int   land    = TerrainHeightFromSums(sum, wx, wz, total, _config);
                const float mtnMask = MountainMask(sum, total, _config);
                const float mesaMask  = MesaMask(sum, total, _config);
                const float mesaCliff = ApronMask(mesaMask, _config.mesaApron, _config.mesaApronThresh);

                // Raw meander (for tunnels) 
                float profile = RiverProfile(wx, wz, _config);
                if (!_config.taigaRivers) profile *= 1.0f - float(sum.taiga) / total;

                // mountains and mesa ignore max height, rivers tunnel isntead after above blocks threshold
                const bool  boringRegion = (biome == BIOME::MOUNTAINS && mtnMask  > _config.tunnelMaskThreshMtn)
                                        || (biome == BIOME::MESA      && mesaMask > _config.tunnelMaskThreshMesa);
                const float riverAllow   = boringRegion ? 1.0f : std::clamp(float(_config.riverMaxHeight - land) / _config.riverFade, 0.0f, 1.0f);
                const float valleyTerr   = profile * riverAllow;

                int waterTop   = _config.waterLevel;
                int terrHeight = land;
                int capBase    = -1;   

                if (profile > 0.0f)
                {
                    const int channelBed = _config.riverLevel - _config.riverDepth;

                    // Tunnel ceiling
                    const float arch    = _config.riverArchHeight * Smooth(profile);
                    const float jit     = FBM(wx / _config.riverCeilScale, wz / _config.riverCeilScale, _config.seed + 960u, 2) * _config.riverCeilJitter;
                    const int   ceiling = _config.riverLevel + static_cast<int>(arch) - static_cast<int>(jit);

                    // check how much blocks above the river, if more than threshold stop smoothing valluy and bore into the terrain
                    const int  boreRise  = (biome == BIOME::MESA) ? _config.tunnelRiseMesa : _config.tunnelRiseMtn;
                    const int  boreFloor = _config.riverLevel + boreRise;
                    const bool walledUp  = boringRegion && land > boreFloor;

                    if (_config.riverTunnels && walledUp
                        && ceiling > _config.riverLevel && land > ceiling)
                    {
                        // carve a channel arched void through the mountain or mesa
                        const float floorF = Lerp(static_cast<float>(_config.riverLevel), static_cast<float>(channelBed), profile);
                        terrHeight = static_cast<int>(floorF);
                        waterTop   = std::max(_config.waterLevel, _config.riverLevel);
                        capBase    = ceiling;
                    }
                    else if (valleyTerr > 0.0f && !walledUp)
                    {
                        const RiverCarve riverCarve = OpenRiverCarve(land, valleyTerr, boringRegion, biome, _config);
                        terrHeight = riverCarve.terrHeight;
                        waterTop   = riverCarve.waterTop;
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
                    // specifical exception for mesa
                    else if (biome == BIOME::MESA && mesaCliff > 0.0f && y < terrHeight)
                    {
                        block = MesaStrata(y, wx, wz, _config);
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
                        else if (biome == BIOME::MESA && mesaCliff > 0.0f)
                        {
                            // banded mesa look
                            block = MesaStrata(terrHeight, wx, wz, _config);
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

                            // oasis grass banks only in desert
                            const BLOCK beachBlock = (biome == BIOME::DESERT && _config.desertRiverGrass) ? BLOCK::GRASS : BLOCK::SAND;

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

                // River-tunnel cap
                if (capBase >= 0)
                {
                    const bool mesaCap = (biome == BIOME::MESA);   
                    for (int y = capBase + 1; y <= land && y < kSizeY; ++y)
                    {
                        BLOCK block;
                        if (y == land)
                            block = mesaCap ? MesaStrata(land, wx, wz, _config)
                                            : MountainSurface(land, wx, wz, _config);
                        else
                            block = mesaCap ? MesaStrata(y, wx, wz, _config)
                                            : StoneAt(wx, y, wz, _config);

                        // Formations on the tunnel ceiling & upper walls
                        if (y < land && y <= capBase + _config.calciteBand
                            && HashFloat3(wx, y, wz, _config.seed + 970u) < _config.calciteChance)
                        {
                            const bool primary = HashFloat3(wx, y, wz, _config.seed + 971u) < _config.dripstoneFraction;
                            if (mesaCap)
                                // gold ore accents in the terracotta 
                                block = primary ? BLOCK::GOLD_ORE : MesaStrata(y, wx, wz, _config);
                            else
                                block = primary ? BLOCK::DRIPSTONE : BLOCK::CALCITE;
                        }
                        _chunk.Set(x, y, z, static_cast<BlockId>(block));
                    }
                }

                // Vegetation
                if (!underWater && capBase < 0)
                {
                    const BLOCK surface = static_cast<BLOCK>(_chunk.At(x, terrHeight, z));

                    PlaceGroundCover(_chunk, x, terrHeight, z, wx, wz, biome, surface, _config);
                }
            }
        }

        // Trees override column fill
        PlaceTrees(_chunk, _config);
    }
}
