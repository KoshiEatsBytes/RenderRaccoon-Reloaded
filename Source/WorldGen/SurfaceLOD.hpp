
#pragma once
#include <vector>
#include <climits>
#include <algorithm>

#include "WorldGen.hpp"
#include "WorldGenConfig.h"
#include "Helpers/Printer.hpp"
#include "Render/Voxels/SurfaceMesher.h"
#include "Voxels/ChunkData.h"
#include "Vegetation.hpp"

namespace WORLDGEN
{
    struct SurfaceField
    {
        // stride level and sample amount
        int level = 0;
        int dim   = 0;

        std::vector<RR::CHUNK::BLOCK> block;
        std::vector<RR::CHUNK::BLOCK> sideColumn;
        std::vector<RR::LodTreeProxy> trees;
        std::vector<BIOME>            biome;
        std::vector<int>              height;
    };

    // distant surface skin for one chunk only, no fill or rivers/tree or veg
    inline SurfaceField ExtractSurface(RR::CHUNK::Coord _cords, int _level, int _footprint, int _coreEdges, const WorldGenConfig& _cfg)
    {
        using namespace RR::CHUNK;
        const bool collectProxies = _level >= 1 && _level <= _cfg.proxyMaxLevel;
        const bool riverCarve     = _level >= 1 && _level <= _cfg.lodRiverCarveMaxLevel;

        const int stride     = 1 << _level;
        const int spanBlocks = _footprint * kSizeX;
        const int nCells     = spanBlocks >> _level;
        const int padG       = nCells + 2;
        const int apron      = stride;
        const int span       = spanBlocks + 2 * apron;
        const int originX    = _cords.x * kSizeX;
        const int originZ    = _cords.z * kSizeZ;
        const int margin  = _cfg.biomeBlendRadius;
        assert(nCells >= 1 && "stride exceeds node span");

        // build biome and blend once per tile
        const BiomeGrid              grid = BuildBiomeGridSpan(originX - apron, originZ - apron, span, margin, _cfg);
        const std::vector<BlendSums> sums = BuildBlendSumsSpan(grid, span, _cfg);
        const int                    total = (2 * margin + 1) * (2 * margin + 1);

        SurfaceField field;
        field.level = _level;
        field.dim   = padG;
        field.height    .resize(padG * padG);
        field.block     .resize(padG * padG);
        field.sideColumn.resize(padG * padG * RR::kLodBandDepth);
        field.biome     .resize(padG * padG);

        for (int sz = -1; sz <= nCells; ++sz)
        {
            for (int sx = -1; sx <= nCells; ++sx)
            {
                const bool rendered = sx >= 0 && sx < nCells && sz >= 0 && sz < nCells;
                const int  idx      = (sx + 1) + (sz + 1) * padG;

                // forward footprint, coll this plateou covers
                const int lxMin = sx * stride;
                const int lzMin = sz * stride;

                // scan stride, keep tallest column
                int bestLand   = INT_MIN;
                int bestLx     = lxMin;
                int bestLz     = lzMin;
                int minReal    = INT_MAX;
                int canopyArea = 0;

                // river state
                long  sumHeight    = 0;
                float bestProfile  = 0.0f;
                int   colCount     = 0;
                bool  riverTouched = false;

                const std::size_t treeStart = field.trees.size();

                for (int dz = 0; dz < stride; ++dz)
                {
                    const int lz = lzMin + dz;

                    for (int dx = 0; dx < stride; ++dx)
                    {
                        const int lx = lxMin + dx;

                        const int wx = originX + lx;
                        const int wz = originZ + lz;

                        const BlendSums& sum       = sums[(lx + apron) + (lz + apron) * span];
                        const int        land      = TerrainHeightFromSums(sum, wx, wz, total, _cfg);
                        const BIOME      currBiome = grid.At(wx, wz);

                        int   riverHeight = land;
                        float valleyTerr  = 0.0f;

                        if (_level >= 1)
                        {
                            float profile = RiverProfile(wx, wz, _cfg);

                            if (!_cfg.taigaRivers) profile *= 1.0f - static_cast<float>(sum.taiga) / total;

                            if (profile > 0.0f)
                            {
                                const float riverAllow = std::clamp(static_cast<float>(_cfg.riverMaxHeight - land) /
                                                                    _cfg.riverFade, 0.0f, 1.0f);
                                valleyTerr = profile * riverAllow;

                                if (valleyTerr > 0.0f)
                                {
                                    // save widest river point
                                    bestProfile = std::max(bestProfile, valleyTerr);

                                    if (riverCarve)
                                    {
                                        // extract river, boring disabled
                                        const RiverCarve rivCarve = OpenRiverCarve(land, valleyTerr, false, currBiome, _cfg);

                                        riverHeight = rivCarve.terrHeight;
                                        riverTouched = true;
                                    }
                                }
                            }
                        }

                        // Finds best for cell
                        if (land > bestLand)
                        {
                            bestLand = land;
                            bestLx   = lx;
                            bestLz   = lz;
                        }

                        // Soften water line
                        sumHeight += riverHeight;
                        // depest surface in the footprint
                        minReal   = std::min(minReal, riverHeight);
                        ++colCount;

                        // Push back proxies into field
                        if (land >= _cfg.waterLevel && valleyTerr <= 0.0f)
                        {
                            if (TreeSpawnRoll(wx, wz, currBiome, _cfg))
                            {
                                const int radius = GetTreeShape(GetVegTypes(currBiome).tree).leafRadius;
                                canopyArea += (2 * radius + 1) * (2 * radius + 1);

                                // push the geometric proxy only for rendered cells in the proxy rings
                                if (rendered && collectProxies &&
                                    HashFloat(wx, wz, _cfg.seed + 1313u) < ProxyKeep(_level, _cfg))
                                {
                                    const BiomeVeg&  biomeVeg = _cfg.biomeVegetation[static_cast<int>(currBiome)];
                                    const TreeShape& shape    = GetTreeShape(GetVegTypes(currBiome).tree);

                                    const int totalH  = PickHeight(HashU32(wx, wz, _cfg.seed + 1314u),
                                                                   biomeVeg.treeMinHeight, biomeVeg.treeMaxHeight);
                                    const int canopyH = std::max(1, static_cast<int>(totalH * shape.canopyFrac));
                                    const int trunkH  = std::max(0, totalH - canopyH);

                                    field.trees.push_back(
                                        { lx, lz, land, trunkH, canopyH, shape.leafRadius,
                                          shape.crownTopFrac, shape.log, GetVegTypes(currBiome).proxyCanopy });
                                }
                            }
                            else if (rendered && collectProxies && CactusSpawnRoll(wx, wz, currBiome, _cfg) &&
                                     HashFloat(wx, wz, _cfg.seed + 1313u) < ProxyKeep(_level, _cfg))
                            {
                                const int cactusH = 3 + static_cast<int>(HashFloat(wx, wz, _cfg.seed + 1002u) * 3.0f);
                                field.trees.push_back(
                                    { lx, lz,
                                        land, cactusH,
                                        0, 0, 0.0f,
                                        BLOCK::CACTUS, BLOCK::CACTUS
                                    });
                            }
                        }
                    }
                }

                // recomputer render height if river
                const int renderHeight = riverTouched ? static_cast<int>(sumHeight / std::max(colCount, 1))
                                                      : bestLand;

                // anchor this cells proxies to the plateau the skin renders
                for (std::size_t i = treeStart; i < field.trees.size(); ++i)
                {
                    field.trees[i].baseY = renderHeight;
                }

                // classify winning column
                const int   wx          = originX + bestLx;
                const int   wz          = originZ + bestLz;
                const BIOME biome       = grid.At(wx, wz);
                const int   land        = renderHeight;
                const int   riverWaterY = std::max(_cfg.waterLevel, _cfg.riverLevel);
                const bool  riverWater  = riverCarve && bestProfile >= _cfg.lodRiverWetProfile;

                const BlendSums& sum = sums[(bestLx + apron) + (bestLz + apron) * span];

                int   surfaceY = land;
                BLOCK block;

                enum STRATA_KIND
                {
                    FLAT,
                    STONE,
                    MESA_BANDS
                };

                STRATA_KIND kind = STRATA_KIND::FLAT;
                BLOCK       flat = GetBiome(biome).subsurface;

                const bool frozen     = _cfg.iceEnabled && (biome == BIOME::TAIGA || biome == BIOME::TUNDRA);
                const bool riverPaint = _level > _cfg.lodRiverCarveMaxLevel && bestProfile >= _cfg.lodRiverPaintThresh;

                // claffication head
                if (riverWater)
                {
                    // flat ribbon at riverLevel
                    surfaceY = riverWaterY;

                    block = frozen ? BLOCK::ICE : BLOCK::WATER;
                }
                else if (land < _cfg.waterLevel)
                {
                    surfaceY = _cfg.waterLevel;
                    block    = BLOCK::WATER;
                }
                else if (biome == BIOME::MOUNTAINS)
                {
                    block = MountainSurface(land, wx, wz, _cfg);

                    // snow above, and keep strata
                    kind = STRATA_KIND::STONE;
                }
                else
                {
                    const float mesaMask  = MesaMask(sum, total, _cfg);
                    const float mesaCliff = ApronMask(mesaMask, _cfg.mesaApron, _cfg.mesaApronThresh);

                    if (biome == BIOME::MESA && mesaCliff > 0.0f)
                    {
                        block = MesaStrata(land, wx, wz, _cfg);
                        kind = STRATA_KIND::MESA_BANDS;
                    }
                    else
                    {
                        block = GetBiome(biome).surface;

                        if (GetBiome(biome).cliffEligible)
                        {
                            kind = STRATA_KIND::STONE;
                        }
                    }
                }

                const BiomeVegTypes& veg = GetVegTypes(biome);
                int canopyDepth = 0;

                if (!riverPaint && _level > _cfg.proxyMaxLevel)
                {
                    // canopy raise per biome veg
                    if (veg.canopy != BLOCK::AIR && land >= _cfg.waterLevel &&
                        canopyArea > stride * stride * _cfg.lodCanopyCoverage)
                    {
                        const BiomeVeg& vd = _cfg.biomeVegetation[static_cast<int>(biome)];
                        // canopy properties per-biome
                        canopyDepth = (vd.treeMinHeight + vd.treeMaxHeight) / 2;
                        surfaceY   += canopyDepth;
                        block       = veg.canopy;
                    }
                }

                // caluclates most top block
                BLOCK topBlock = block;
                if (riverPaint)
                {
                    if (frozen)
                    {
                        topBlock = BLOCK::ICE;
                    }
                    else
                    {
                        topBlock =  BLOCK::WATER;
                    }
                }

                const int colBase    = idx * RR::kLodBandDepth;

                // small helper for side bands
                auto bandBlock = [&](int _depth) -> BLOCK
                {
                    const int worldY = surfaceY - _depth;

                    switch (kind)
                    {
                        case STONE:
                            return StoneAt(wx, worldY, wz, _cfg);

                        case MESA_BANDS:
                            return MesaStrata(worldY, wx, wz, _cfg);

                        default:
                            return flat;
                    }
                };

                bool coreFacing = false;
                if (!rendered)
                {
                    // reads bitmap
                    if (sx == -1     && (_coreEdges & 0x1)) coreFacing = true;
                    if (sx == nCells && (_coreEdges & 0x2)) coreFacing = true;
                    if (sz == -1     && (_coreEdges & 0x4)) coreFacing = true;
                    if (sz == nCells && (_coreEdges & 0x8)) coreFacing = true;
                }

                field.height[idx] = coreFacing ? minReal : surfaceY;

                if (rendered)
                {
                    // Inject sides into field, 0 is surface block
                    field.sideColumn[colBase] = block;

                    const int fillDepth = _level <= _cfg.lodBandMaxLevel ? RR::kLodBandDepth : 2;
                    for (int depth = 1; depth < fillDepth; ++depth)
                    {
                        if (depth < canopyDepth)
                        {
                            field.sideColumn[colBase + depth] = veg.canopy;
                        }
                        else
                        {
                            field.sideColumn[colBase + depth] = bandBlock(depth);
                        }
                    }

                    // populate the rest
                    field.block[idx]  = topBlock;
                    field.biome[idx]  = biome;
                }
            }
        }

        return field;
    }
}










