
#pragma once
#include <vector>

#include "BiomeID.h"
#include "Noise.hpp"
#include "WorldGenConfig.h"

namespace WORLDGEN
{
    // Distinct salts so each roll is an independent stream
    constexpr uInt32 kSaltTemp  = 811u;
    constexpr uInt32 kSaltHumid = 822u;
    constexpr uInt32 kSaltRare  = 833u;
    constexpr uInt32 kSaltMount = 844u;

    struct BiomeGrid
    {
        int ox = 0;
        int oz = 0;
        int w = 0;
        std::vector<BIOME> cells;

        BIOME At(int _wx, int _wz) const
        {
            return cells[(_wx - ox) + (_wz - oz) * w];
        }
    };


    // Coarse-cell biome: random category per cell
    inline BIOME BaseBiome(int _cx, int _cz, const WorldGenConfig& _config)
    {
        // Hash and check for mountains
        if (HashFloat(_cx, _cz, _config.seed + kSaltMount) < _config.mountainChance)
        {
            return BIOME::MOUNTAINS;
        }

        const float temp  = HashFloat(_cx, _cz, _config.seed + kSaltTemp);
        const float humid = HashFloat(_cx, _cz, _config.seed + kSaltHumid);

        // COLD
        if (temp < _config.tempCold)
        {
            // Tundra or taiga
            if (humid < _config.TundraHumidThresh) return BIOME::TUNDRA;
            return BIOME::TAIGA;
        }
        // HOT
        if (temp > _config.tempHot)
        {
            if (HashFloat(_cx, _cz, _config.seed + kSaltRare) > _config.redDesertRarity) {
                return BIOME::RED_DESERT;
            }

            // Desert or savanna;
            if (humid < _config.desertHumidThresh) return BIOME::DESERT;
            return BIOME::SAVANNA;
        }
        // TEMPERATE
        // Plains or forest
        if (humid < _config.PlainsHumidThresh) return BIOME::PLAINS;
        return BIOME::FOREST;
    }

    inline int FloorDiv(int _a, int _b)
    {
        int q = _a / _b;
        if ((_a % _b != 0) && ((_a < 0) != (_b < 0))) --q;
        return q;
    }

    // Biome at a world column - Pure function
    inline BIOME BiomeAt(int _wx, int _wz, const WorldGenConfig& _config)
    {
        const float fx = static_cast<float>(_wx);
        const float fz = static_cast<float>(_wz);

        // two independent warp fields
        const float dx = (FBM(fx/_config.biomeWarpScale, fz/_config.biomeWarpScale, _config.seed + 901u, _config.biomeWarpOct) - 0.5f) * 2.0f * _config.biomeWarpAmp;
        const float dz = (FBM(fx/_config.biomeWarpScale, fz/_config.biomeWarpScale, _config.seed + 902u, _config.biomeWarpOct) - 0.5f) * 2.0f * _config.biomeWarpAmp;

        const int cx = FloorDiv(static_cast<int>(std::floor(fx + dx)), _config.biomeRegionScale);
        const int cz = FloorDiv(static_cast<int>(std::floor(fz + dz)), _config.biomeRegionScale);
        return BaseBiome(cx, cz, _config);
    }

    // CELL RNG

    inline uInt32 CellRand(int _x, int _z, uInt32 _seed, uInt32 _salt)
    {
        uInt32 hash = RR::CHUNK::Hash(_x, _z) ^ (_seed * 0x9e3779b1u) ^ (_salt * 0x85ebca77u);

        hash ^= hash >> 16;
        hash *= 0x7feb352du;
        hash ^= hash >> 15;
        hash *= 0x846ca68bu;
        hash ^= hash >> 16;

        return hash;
    }

    inline BIOME Choose2(BIOME a, BIOME b, uInt32 r)
    {
        return (r & 1u) ? b : a;
    }

    inline BIOME ChooseMode(BIOME _a, BIOME _b, BIOME _c, BIOME _d, uInt32 _r)   // majority, random tie-break
    {
        if (_b == _c && _c == _d) return _b;
        if (_a == _b && _a == _c) return _a;
        if (_a == _b && _a == _d) return _a;
        if (_a == _c && _a == _d) return _a;
        if (_a == _b && _c != _d) return _a;
        if (_a == _c && _b != _d) return _a;
        if (_a == _d && _b != _c) return _a;
        if (_b == _c && _a != _d) return _b;
        if (_b == _d && _a != _c) return _b;
        if (_c == _d && _a != _b) return _c;

        switch (_r & 3u)
        {
            case 0:
                return _a;

            case 1:
                return _b;

            case 2:
                return _c;

            default:
                return _d;
        }
    }

    // BASE LAYER COARSE CELL
    inline std::vector<BIOME> BaseArea(int _x, int _z, int _w, int _h, const WorldGenConfig& _config)
    {
        std::vector<BIOME> out(static_cast<size_t>(_w) * _h);

        for (int j = 0; j < _h; ++j)
        {
            for (int i = 0; i < _w; ++i)
            {
                out[i + j * _w] = BaseBiome(_x + i, _z + j, _config);
            }
        }

        return out;
    }

    // fw dec because those two are mutually recursive
    inline std::vector<BIOME> BuildArea(int _level, int _x, int _z, int _w, int _h, const WorldGenConfig& _config);

    inline std::vector<BIOME> ZoomArea(int _level, int _x, int _z, int _w, int _h, const WorldGenConfig& _config)
    {
        const int px = _x >> 1;
        const int pz = _z >> 1;
        const int pw = (_w >> 1) + 2;
        const int ph = (_h >> 1) + 2;

        const std::vector<BIOME> parent = BuildArea(_level - 1, px, pz, pw, ph, _config);

        const int tmpW = (pw - 1) * 2, tmpH = (ph - 1) * 2;
        std::vector<BIOME> tmp(static_cast<size_t>(tmpW) * tmpH);
        const uInt32 salt = static_cast<uInt32>(_level);

        for (int pj = 0; pj < ph - 1; ++pj)
        {
            for (int pi = 0; pi < pw - 1; ++pi)
            {
                const BIOME a = parent[ pi      +  pj      * pw];
                const BIOME b = parent[(pi + 1) +  pj      * pw];
                const BIOME c = parent[ pi      + (pj + 1) * pw];
                const BIOME d = parent[(pi + 1) + (pj + 1) * pw];

                const int ox = (px + pi) * 2, oz = (pz + pj) * 2;
                const int ti = pi * 2, tj = pj * 2;

                tmp[ ti      +  tj      * tmpW] = a;
                tmp[ ti      + (tj + 1) * tmpW] = Choose2(a, c, CellRand(ox,     oz + 1, _config.seed, salt));
                tmp[(ti + 1) +  tj      * tmpW] = Choose2(a, b, CellRand(ox + 1, oz,     _config.seed, salt));
                tmp[(ti + 1) + (tj + 1) * tmpW] = ChooseMode(a, b, c, d, CellRand(ox + 1, oz + 1, _config.seed, salt));
            }
        }

        const int offX = _x - px * 2;
        const int offZ = _z - pz * 2;

        std::vector<BIOME> out(static_cast<size_t>(_w) * _h);

        for (int j = 0; j < _h; ++j)
        {
            for (int i = 0; i < _w; ++i)
            {
                out[i + j * _w] = tmp[(offX + i) + (offZ + j) * tmpW];
            }
        }

        return out;
    }

    inline std::vector<BIOME> BuildArea(int _level, int _x, int _z, int _w, int _h, const WorldGenConfig& _config)
    {
        if (_level <= 0) return BaseArea(_x, _z, _w, _h, _config);
        return ZoomArea(_level, _x, _z, _w, _h, _config);
    }

    inline BIOME BiomeAtZoom(int _wx, int _wz, const WorldGenConfig& _config)
    {
        return BuildArea(_config.biomeZoomLevels, _wx, _wz, 1, 1, _config)[0];
    }

    // fw dcl function below
    inline std::vector<BIOME> SmoothArea(int _pass, int _x, int _z, int _w, int _h, const WorldGenConfig& _config);

    inline std::vector<BIOME> FinalArea(int _x, int _z, int _w, int _h, const WorldGenConfig& _config)
    {
        if (_config.biomeSmoothPasses <= 0)
        {
            return BuildArea(_config.biomeZoomLevels, _x, _z, _w, _h, _config);
        }
        return SmoothArea(_config.biomeSmoothPasses, _x, _z, _w, _h, _config);
    }

    inline std::vector<BIOME> SmoothArea(int _pass, int _x, int _z, int _w, int _h, const WorldGenConfig& _config)
    {
        const std::vector<BIOME> parent = _pass <= 1 ?
            BuildArea(_config.biomeZoomLevels, _x - 1, _z - 1, _w + 2, _h + 2, _config) :
            SmoothArea(_pass - 1, _x - 1, _z - 1, _w + 2, _h + 2, _config);

        const int pw = _w + 2;
        const uInt32 salt = 5000u + static_cast<uInt32>(_pass);

        std::vector<BIOME> out(static_cast<size_t>(_w) * _h);
        for (int j = 0; j < _h; ++j)
        {
            for (int i = 0; i < _w; ++i)
            {
                const BIOME center = parent[(i + 1) + (j + 1) * pw];
                const BIOME left   = parent[ i      + (j + 1) * pw];
                const BIOME right  = parent[(i + 2) + (j + 1) * pw];
                const BIOME up     = parent[(i + 1) +  j      * pw];
                const BIOME down   = parent[(i + 1) + (j + 2) * pw];

                const bool xm = (left == right), zm = (up == down);
                BIOME r;
                if (xm && zm) r = (CellRand(_x + i, _z + j, _config.seed, salt) & 1u) ? left : up;  // both axes agree → coin-flip
                else if (xm)  r = left;
                else if (zm)  r = up;
                else          r = center;                                                     // no agreement → keep
                out[i + j * _w] = r;
            }
        }

        return out;
    }

    inline BiomeGrid BuildBiomeGrid(int _chunkX, int _chunkZ, int _margin, const WorldGenConfig& _config)
    {
        const int ox = _chunkX * RR::CHUNK::kSizeX - _margin;
        const int oz = _chunkZ * RR::CHUNK::kSizeZ - _margin;
        const int w  = RR::CHUNK::kSizeX + 2 * _margin;
        const int h  = RR::CHUNK::kSizeZ + 2 * _margin;
        return BiomeGrid{ ox, oz, w, FinalArea(ox, oz, w, h, _config) };
    }
}