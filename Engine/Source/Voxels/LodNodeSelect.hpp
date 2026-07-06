
#pragma once
#include<vector>
#include<array>
#include<unordered_map>
#include<unordered_set>
#include<algorithm>

#include "ChunkData.h"
#include "Helpers/Printer.hpp"

namespace RR
{
    struct LodNodeKey
    {
        // min corner for footprint
        CHUNK::Coord origin;
        int level     = 0;
        int footprint = 1;

        bool operator==(const LodNodeKey &) const = default;
    };

    struct LodNodeKeyHash
    {
        using sizeT = std::size_t;

        sizeT operator()(const LodNodeKey& _key) const
        {
            sizeT hash = CHUNK::CoordHash{}(_key.origin);

            hash ^= std::hash<int>{}(_key.level)     + 0x9e3779b9u + (hash << 6) + (hash >> 2);
            hash ^= std::hash<int>{}(_key.footprint) + 0x9e3779b9u + (hash << 6) + (hash >> 2);
            return hash;
        }
    };

    struct RingParams
    {
        int   coreRadius  = 16;
        float ringGrowth  = 2.0f;
        int   maxLevel    = 4;
        int   nodingStart = 2;   // first aggregated level
        int   meshRadius  = 16;  // RD
        int   hysteresis  = 2;
        int   ringBase    = 24;  // ring width anchor
    };

    using NodeSet = std::unordered_set<LodNodeKey, LodNodeKeyHash>;

    // Floor to lowest e.g. -1/2 -> -1
    inline int FloorDiv(int _a, int _b)
    {
        int result = 0;

        if (_a >= 0)
        {
            result = _a / _b;
        }
        else
        {
            result = -((-_a + _b - 1) / _b);
        }

        return result;
    }

    // Snap onto power of tweo grid
    inline int SnapDown(int _c, int _sizePow2)
    {
        return _c & ~(_sizePow2 - 1);
    }

    // Default anchor, regardless of core radius 
    inline constexpr int kRingBaseRadius = 24;

    // edge ring calculated with ring growth
    inline int RingEdge(int _level, const RingParams& _params)
    {
        float edge = static_cast<float>(_params.ringBase);

        for (int l = 0; l < _level; ++l)
        {
            edge *= _params.ringGrowth;
        }

        return _params.coreRadius - _params.ringBase + static_cast<int>(edge);
    }

    // return lod level per distance
    inline int LevelForDistance(int _dist, const RingParams& _params)
    {
        if (_dist <= _params.coreRadius) return 0;

        int level = 1;
        while (_dist > RingEdge(level, _params) && level < _params.maxLevel)
        {
            ++level;
        }
        return level;
    }

    inline int NodeFootprint(int _level, const RingParams& _params)
    {
        if (_level >= _params.nodingStart)
        {
            return 1 << (_level - _params.nodingStart + 1);
        }

        return 1;
    }

    // Chebyshev distance from chunk centre to nearest of the footprint
    inline int NearestDist(CHUNK::Coord _center, CHUNK::Coord _origin, int _footprintSize)
    {
        // Distance from a single coordinate to the span
        const auto DistanceToSpan = [](int _coord, int _min, int _size) -> int
        {
            const int max = _min + _size - 1;
            // before the span
            if (_coord < _min) return _min - _coord;
            // after the span
            if (_coord > max)  return _coord - max;
            // inside the span
            return 0;
        };

        const int dx = DistanceToSpan(_center.x, _origin.x, _footprintSize);
        const int dz = DistanceToSpan(_center.z, _origin.z, _footprintSize);
        return std::max(dx, dz);
    }

    // Chebyshev distance from chunk centre to fartherst of the footprint
    inline int FarthestDist(CHUNK::Coord _center, CHUNK::Coord _origin, int _footprintSize)
    {
        // Distance from a single coordinate to the edge of the span
        const auto DistanceToFarEdge = [](int _coord, int _min, int _size) -> int
        {
            const int max = _min + _size - 1;
            return std::max(std::abs(_coord - _min), std::abs(_coord - max));
        };

        const int dx = DistanceToFarEdge(_center.x, _origin.x, _footprintSize);
        const int dz = DistanceToFarEdge(_center.z, _origin.z, _footprintSize);
        return std::max(dx, dz);
    }

    inline bool HasChildren(const LodNodeKey& _node, const RingParams& _params, const NodeSet& _prev)
    {
        const int childFootprint = NodeFootprint(_node.level - 1, _params);

        // A node footprint is up to 4 children
        for (int childZ = 0; childZ <= 1; ++childZ)
        {
            for (int childX = 0; childX <= 1; ++childX)
            {
                const CHUNK::Coord childOrigin
                {
                    _node.origin.x + childX * childFootprint,
                    _node.origin.z + childZ * childFootprint
                };

                if (_node.level > _params.nodingStart)
                {
                    // Above the noding threshold, if a child exists sits a -1
                    if (_prev.contains({ childOrigin, _node.level - 1, childFootprint }))
                    {
                        return true;
                    }
                }
                else
                {
                    // At/below noding threhs children are per-chunk nodes whose level
                    // was recomputed from distance
                    for (int level = 1; level <= _params.nodingStart; ++level)
                    {
                        if (_prev.contains({ childOrigin, level, 1 }))
                        {
                            return true;
                        }
                    }
                }
            }
        }

        return false;
    }

    inline void Descend(const LodNodeKey& _node, CHUNK::Coord _centre, const RingParams& _params,
                        const NodeSet* _prev, std::vector<LodNodeKey>& _out)
    {
        const int fPrint = NodeFootprint(_node.level, _params);

        const LodNodeKey node {
            _node.origin,
            _node.level,
            fPrint
        };

        const int dNear  = NearestDist  (_centre, node.origin, fPrint);

        // beyond rd
        if (dNear > _params.meshRadius) return;
        // in core
        if (FarthestDist(_centre, node.origin, fPrint) <= _params.coreRadius) return;

        // per-chunk floor
        if (fPrint == 1)
        {
            if (dNear <= _params.coreRadius) return;

            int lvl = LevelForDistance(dNear, _params);

            // sticky level
            if (_prev)
            {
                for (int lv = 1; lv <= _params.maxLevel; ++lv)
                {
                    if (!_prev->contains({node.origin, lv, 1})) continue;

                    if (lvl > lv)
                    {
                        lvl = LevelForDistance(dNear - _params.hysteresis, _params);
                    }
                    else if (lvl < lv)
                    {
                        lvl = LevelForDistance(dNear + _params.hysteresis, _params);
                    }
                    break;
                }
            }

            // Hysteresis must not break l0
            lvl = std::max(lvl, 1);

            _out.push_back({node.origin, lvl, 1});
            return;
        }

        // node only valid at its own level id even its nearest chunk is far enough to belong to level -1
        int refineEdge = RingEdge(_node.level -1, _params);

        // ring hysteresis, stick to whatever states already exists
        if (_prev)
        {
            // stay coarse longer
            if (_prev->contains(node))
            {
                refineEdge -= _params.hysteresis;
            }
            // stay fine longer
            else if (HasChildren(_node, _params, *_prev))
            {
                refineEdge += _params.hysteresis;
            }
        }

        if (dNear > refineEdge)
        {
            _out.push_back(node);
            return;
        }

        // Too close, refine down to 4 children
        const int childFootprint = fPrint / 2;
        for (int dz = 0; dz <= 1; ++dz)
        {
            for (int dx = 0; dx <= 1; ++dx)
            {
                Descend({{
                        _node.origin.x + dx*childFootprint,
                        _node.origin.z + dz*childFootprint
                         },
                    _node.level - 1},
                    _centre, _params, _prev, _out);
            }
        }
    }

    // builds grid for top-level, then descends into each and assigns final lod level for that node
    inline void SelectNodes(CHUNK::Coord _centre, const RingParams& _params,
                            const NodeSet* _prev, std::vector<LodNodeKey>& _out)
    {
        const int topFootprint = NodeFootprint(_params.maxLevel, _params);
        const int xMin = SnapDown(_centre.x - _params.meshRadius, topFootprint);
        const int zMin = SnapDown(_centre.z - _params.meshRadius, topFootprint);

        for (int gz = zMin; gz <= _centre.z + _params.meshRadius; gz += topFootprint)
        {
            for (int gx = xMin; gx <= _centre.x + _params.meshRadius; gx += topFootprint)
            {
                Descend({
                    {gx, gz},
                    _params.maxLevel},
                    _centre, _params, _prev, _out);
            }
        }
    }
}
