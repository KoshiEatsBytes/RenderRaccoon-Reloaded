
#pragma once
#include <cstdint>
#include <cmath>
#include "Voxels/ChunkData.h"

namespace WORLDGEN
{
    using uInt32 = std::uint32_t;

    // Pseudo rando value - seeded
    inline float HashFloat(int _x, int _z, uInt32 _seed)
    {
        // scale seed
        uInt32 hash = RR::CHUNK::Hash(_x, _z) ^ (_seed * 0x9e3779b1u);

        // full avalanche
        hash ^= hash >> 16;
        hash *= 0x7feb352du;
        hash ^= hash >> 15;
        hash *= 0x846ca68bu;
        hash ^= hash >> 16;

        return static_cast<float>(hash) / 4294967295.0f;
    }

    inline float HashFloat3(int _x, int _y, int _z, uInt32 _seed)
    {
        uInt32 hash = RR::CHUNK::Hash(_x, _z)
                ^ (static_cast<uInt32>(_y) * 0x85ebca77u)
                ^ (_seed * 0x9e3779b1u);

        hash ^= hash >> 16;
        hash *= 0x7feb352du;
        hash ^= hash >> 15;
        hash *= 0x846ca68bu;
        hash ^= hash >> 16;
        return static_cast<float>(hash) / 4294967295.0f;
    }

    // smooth step fade
    inline float Smooth(float _t)
    {
        return _t * _t * (3.0f - 2.0f * _t);
    }

    inline float Lerp(float _a, float _b, float _t)
    {
        return _a + (_b - _a) * _t;
    }

    // Bilinear value noise
    inline float ValueNoise(float _x, float _z, uInt32 _seed)
    {
        const int x0 = static_cast<int>(std::floor(_x));
        const int z0 = static_cast<int>(std::floor(_z));
        const float fx = Smooth(_x - x0);
        const float fz = Smooth(_z - z0);

        const float n00 = HashFloat(x0,   z0,   _seed);
        const float n10 = HashFloat(x0+1, z0,   _seed);
        const float n01 = HashFloat(x0,   z0+1, _seed);
        const float n11 = HashFloat(x0+1, z0+1, _seed);

        const float lerp0010 = Lerp(n00, n10, fx);
        const float lerp0111 = Lerp(n01, n11, fx);

        return Lerp(lerp0010, lerp0111, fz);
    }

    // Octaves of value noise, normalized to [0,1]
    // Fractal brownian motion
    inline float FBM(float _x, float _z, uInt32 _seed, int _octaves)
    {
        if (_octaves <= 0) _octaves = 1;
        float sum  = 0.0f;
        float amp  = 1.0f;
        float freq = 1.0f;
        float norm = 0.0f;

        for (int o = 0; o < _octaves; ++o)
        {
            // Sub-Seed octave
            sum += amp * ValueNoise(_x * freq, _z * freq, _seed + static_cast<uInt32>(o));
            norm += amp;
            amp  *= 0.5f;
            freq *= 2.0f;
        }

        return sum / norm;
    }

    // get noise value - but 3D!
    inline float ValueNoise3(float _x, float _y, float _z, uInt32 _seed)
    {
        const int x0 = static_cast<int>(std::floor(_x));
        const int y0 = static_cast<int>(std::floor(_y));
        const int z0 = static_cast<int>(std::floor(_z));
        const float fx = Smooth(_x - x0);
        const float fy = Smooth(_y - y0);
        const float fz = Smooth(_z - z0);

        const float c000 = HashFloat3(x0, y0, z0, _seed);
        const float c100 = HashFloat3(x0+1, y0, z0, _seed);
        const float c010 = HashFloat3(x0, y0+1, z0,  _seed);
        const float c110 = HashFloat3(x0+1, y0+1, z0, _seed);
        const float c001 = HashFloat3(x0, y0, z0+1, _seed);
        const float c101 = HashFloat3(x0+1, y0, z0+1, _seed);
        const float c011 = HashFloat3(x0, y0+1, z0+1, _seed);
        const float c111 = HashFloat3(x0+1, y0+1, z0+1, _seed);

        const float x00 = Lerp(c000,c100,fx);
        const float x10 = Lerp(c010,c110,fx);
        const float x01 = Lerp(c001,c101,fx);
        const float x11 = Lerp(c011,c111,fx);

        return Lerp(Lerp(x00,x10,fy), Lerp(x01,x11,fy), fz);   // trilinear
    }
}
































