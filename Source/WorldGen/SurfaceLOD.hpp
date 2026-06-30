
#pragma once
#include <vector>

#include "WorldGen.hpp"
#include "WorldGenConfig.h"
#include "Helpers/Printer.hpp"
#include "Render/Voxels/SurfaceMesher.h"
#include "Voxels/ChunkData.h"

namespace WORLDGEN
{
    struct SurfaceField
    {
        // stride level and sample amount
        int level = 0;
        int dim   = 0;

        std::vector<RR::CHUNK::BLOCK> block;
        std::vector<RR::CHUNK::BLOCK> side;
        std::vector<BIOME>            biome;
        std::vector<int>              height;
    };

    // distant surface skin for one chunk only, no fill or rivers/tree or veg
    inline SurfaceField ExtractSurface(RR::CHUNK::Coord _cords, int _level, const WorldGenConfig& _cfg)
    {
        using namespace RR::CHUNK;

        const int stride  = 1 << _level;
        const int span    = kSizeX + 1;
        const int dim     = (kSizeX >> _level) + 1;
        const int originX = _cords.x * kSizeX;
        const int originZ = _cords.z * kSizeZ;
        const int margin  = _cfg.biomeBlendRadius;

        // build biome and blend once per tile
        const BiomeGrid              grid = BuildBiomeGridSpan(originX, originZ, span, margin, _cfg);
        const std::vector<BlendSums> sums = BuildBlendSumsSpan(grid, span, _cfg);
        const int                    total = (2 * margin + 1) * (2 * margin + 1);

        SurfaceField field;
        field.level = _level;
        field.dim   = dim;
        field.height.resize(dim * dim);
        field.block.resize(dim * dim);
        field.side.resize(dim * dim);
        field.biome.resize(dim * dim);

        for (int sz = 0; sz < dim; ++sz)
        {
            const int lz = sz * stride;

            for (int sx = 0; sx < dim; ++sx)
            {
                const int lx = sx * stride;
                const int wx = originX + lx;
                const int wz = originZ + lz;

                const BIOME      biome = grid.At(wx, wz);
                const BlendSums& sum = sums[lx + lz * span];
                const int        land = TerrainHeightFromSums(sum, wx, wz, total, _cfg);

                int   surfaceY = land;
                BLOCK block;
                BLOCK side;

                if (land < _cfg.waterLevel)
                {
                    surfaceY = _cfg.waterLevel;
                    block    = BLOCK::WATER;
                    side     = GetBiome(biome).subsurface;
                }
                else if (biome == BIOME::MOUNTAINS)
                {
                    block = MountainSurface(land, wx, wz, _cfg);

                    // snow above, and keep strata
                    side  = (block == BLOCK::SNOW) ? BLOCK::SNOW
                                                   : StoneAt(wx, land - 1, wz, _cfg);
                }
                else
                {
                    const float mesaMask  = MesaMask(sum, total, _cfg);
                    const float mesaCliff = ApronMask(mesaMask, _cfg.mesaApron, _cfg.mesaApronThresh);

                    // survive terracotta bands
                    if (biome == BIOME::MESA && mesaCliff > 0.0f)
                    {
                        block = MesaStrata(land,     wx, wz, _cfg);
                        side  = MesaStrata(land - 1, wx, wz, _cfg);
                    }
                    else
                    {
                        block = GetBiome(biome).surface;
                        side  = GetBiome(biome).cliffEligible ? StoneAt(wx, land - 1, wz, _cfg)
                                                              : GetBiome(biome).subsurface;
                    }
                }

                const int i = sx + sz * dim;
                field.height[i] = surfaceY;
                field.block[i]  = block;
                field.side[i]   = side;
                field.biome[i]  = biome;
            }
        }

        return field;
    }

    // DELETE BEFORE RELEASE
    // Console proof for the LOD surface keystone
    inline void ProveSurfaceLOD(const WorldGenConfig& _cfg)
    {
        using namespace RR::CHUNK;
        const int margin = _cfg.biomeBlendRadius;
        const int total  = (2 * margin + 1) * (2 * margin + 1);

        const Coord coords[] = {
            {0, 0},
            {3, -2},
            {-5, 7}
        };

        bool matchOk = true;
        for (const Coord c : coords)
        {
            const SurfaceField           f = ExtractSurface(c, 0, _cfg);
            const BiomeGrid              g = BuildBiomeGrid(c.x, c.z, margin, _cfg);
            const std::vector<BlendSums> s = BuildBlendSums(g, _cfg);      // span 16

            for (int z = 0; z < kSizeZ && matchOk; ++z)
            {
                for (int x = 0; x < kSizeX; ++x)
                {
                    const int   wx    = c.x * kSizeX + x;
                    const int   wz    = c.z * kSizeZ + z;
                    const BIOME biome = g.At(wx, wz);
                    const int   land  = TerrainHeightFromSums(s[x + z * kSizeX], wx, wz, total, _cfg);
                    const int   refY  = (land < _cfg.waterLevel) ? _cfg.waterLevel : land;

                    const int i = x + z * f.dim;

                    if (f.height[i] != refY || f.biome[i] != biome)
                    {
                        RR::Error("[SurfaceLOD] A mismatch at (", wx, ",", wz,
                                  ") field=", f.height[i], " ref=", refY);
                        matchOk = false;
                        break;
                    }
                }
            }
        }
        if (matchOk)
            RR::Success("[SurfaceLOD] A: level-0 == full-res 16-path on cols 0..15 (incl neg coords)");

        const SurfaceField a = ExtractSurface({2, 2}, 2, _cfg);
        const SurfaceField b = ExtractSurface({2, 2}, 2, _cfg);

        if (a.height == b.height && a.block == b.block && a.biome == b.biome)
            RR::Success("[SurfaceLOD] B: same seed twice -> identical");
        else
            RR::Error("[SurfaceLOD] B: NON-DETERMINISTIC");

        WorldGenConfig bumped = _cfg;
        bumped.seed = _cfg.seed + 1u;
        const SurfaceField d = ExtractSurface({2, 2}, 2, bumped);

        if (a.height != d.height || a.block != d.block || a.biome != d.biome)
            RR::Success("[SurfaceLOD] B: seed+1 -> differs");
        else
            RR::Error("[SurfaceLOD] B: seed+1 produced identical world");

        const bool dimsOk = ExtractSurface({0,0}, 0, _cfg).dim == 17
                         && ExtractSurface({0,0}, 1, _cfg).dim == 9
                         && ExtractSurface({0,0}, 2, _cfg).dim == 5
                         && ExtractSurface({0,0}, 4, _cfg).dim == 2;
        dimsOk ? RR::Success("[SurfaceLOD] C: dims 17/9/5/2 for L0/1/2/4")
               : RR::Error  ("[SurfaceLOD] C: bad dim");
    }

    inline void ProveSurfaceMesher()
    {
        using RR::CHUNK::BLOCK;
        const int dim = 5, level = 2, skirt = 8;
        const std::vector<int>   h(dim * dim, 80);
        const std::vector<BLOCK> b(dim * dim, BLOCK::GRASS);
        const std::vector<BLOCK> s(dim * dim, BLOCK::DIRT);

        const RR::MeshData m = RR::MeshSurface(dim, level, h, b, s, skirt);

        // flat field: cells^2 tops + 4*cells perimeter skirts, all single-sided
        const std::size_t verts = m.vertices.size() / 9;
        const std::size_t quads = (dim - 1) * (dim - 1);
        const std::size_t segs  = (skirt > 0) ? 4 * (dim - 1) : 0;
        const std::size_t expV  = (quads + segs) * 4;
        const std::size_t expI  = (quads + segs) * 6;
        const bool ok = (verts == expV) && (m.indices.size() == expI);

        if (ok)
            RR::Success("[SurfaceMesher] flat L2: ", quads, " quads -> ", verts, " verts / ", m.indices.size(), " indices");
        else
            RR::Error  ("[SurfaceMesher] bad counts: verts=", verts, " idx=", m.indices.size());
    }
}



























