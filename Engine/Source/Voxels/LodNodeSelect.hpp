
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

    inline int RingEdge(int _level, const RingParams& _params)
    {
        float edge = static_cast<float>(_params.coreRadius);

        for (int l = 0; l < _level; ++l)
        {
            edge *= _params.ringGrowth;
        }

        return static_cast<int>(edge);
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

    /// TODO DELETE BEFORE RELEASE PROOF ONLY

    inline bool ProveNodeSelect()
{
    using namespace RR::CHUNK;

    bool allOk   = true;
    int  printed = 0;

    // print the first few failures with context, then go quiet
    const auto fail = [&](auto&&... _ctx)
    {
        allOk = false;
        if (++printed <= 10) Error("[NodeSelect] ", _ctx...);
    };

    // ---- proof 0: helper units (the negative-coord footgun) -------------------------
    if (FloorDiv(-1, 2) != -1 || FloorDiv(-4, 2) != -2 || FloorDiv(3, 2) != 1)
        fail("FloorDiv wrong");

    if (SnapDown(-1, 2) != -2 || SnapDown(-5, 4) != -8 || SnapDown(-8, 4) != -8 ||
        SnapDown( 7, 4) !=  4 || SnapDown( 0, 8) !=  0)
        fail("SnapDown wrong");

    {
        const Coord c{-3, 5}, o{-7, 2};
        const int cd = std::max(std::abs(c.x - o.x), std::abs(c.z - o.z));
        if (NearestDist(c, o, 1) != cd || FarthestDist(c, o, 1) != cd)
            fail("f==1 distance degeneracy broken");
    }

    // ---- config battery (negatives cover proof 3) -----------------------------------
    struct Cfg { Coord centre; RingParams p; };
    const std::vector<Cfg> cfgs = {
        {{   5,   7}, {.coreRadius=16, .ringGrowth=2.0f, .maxLevel=4, .nodingStart=2, .meshRadius= 64, .hysteresis=2}},
        {{   0,   0}, {.coreRadius=16, .ringGrowth=2.0f, .maxLevel=4, .nodingStart=2, .meshRadius= 96, .hysteresis=2}},
        {{  -1,   1}, {.coreRadius=16, .ringGrowth=2.0f, .maxLevel=4, .nodingStart=2, .meshRadius= 96, .hysteresis=2}},
        {{ -37,  -5}, {.coreRadius=16, .ringGrowth=2.0f, .maxLevel=4, .nodingStart=2, .meshRadius=128, .hysteresis=2}},
        {{-1000, 999},{.coreRadius=16, .ringGrowth=2.0f, .maxLevel=4, .nodingStart=2, .meshRadius=128, .hysteresis=2}},
        {{ -37,  42}, {.coreRadius=16, .ringGrowth=2.0f, .maxLevel=4, .nodingStart=3, .meshRadius=128, .hysteresis=2}}, // L3-noding A/B
        {{   9,  -3}, {.coreRadius=16, .ringGrowth=2.0f, .maxLevel=4, .nodingStart=2, .meshRadius= 40, .hysteresis=2}}, // RD too small for nodes
        {{  12,  34}, {.coreRadius=16, .ringGrowth=3.0f, .maxLevel=3, .nodingStart=2, .meshRadius=150, .hysteresis=2}}, // non-pow2 growth
        {{ 123,-456}, {.coreRadius=16, .ringGrowth=2.0f, .maxLevel=4, .nodingStart=2, .meshRadius=384, .hysteresis=2}}, // showpiece
    };

    int cfgIdx = -1;
    for (const Cfg& cfg : cfgs)
    {
        ++cfgIdx;
        const Coord       c = cfg.centre;
        const RingParams& p = cfg.p;

        std::vector<LodNodeKey> keys;
        SelectNodes(c, p, nullptr, keys);

        // ---- proof 6: determinism + no duplicate keys -------------------------------
        std::vector<LodNodeKey> again;
        SelectNodes(c, p, nullptr, again);

        NodeSet set(keys.begin(), keys.end());
        if (set.size() != keys.size())
            fail("cfg ", cfgIdx, ": duplicate keys emitted");
        if (NodeSet(again.begin(), again.end()) != set)
            fail("cfg ", cfgIdx, ": non-deterministic");

        // ---- proof 1: exact partition ------------------------------------------------
        std::unordered_map<Coord, int, CoordHash> paint;
        paint.reserve(static_cast<std::size_t>(2 * p.meshRadius + 1) * (2 * p.meshRadius + 1));

        for (const LodNodeKey& k : keys)
            for (int dz = 0; dz < k.footprint; ++dz)
                for (int dx = 0; dx < k.footprint; ++dx)
                    ++paint[{k.origin.x + dx, k.origin.z + dz}];

        for (const auto& [coord, count] : paint)
        {
            if (count != 1)
                fail("cfg ", cfgIdx, ": double cover at ", coord.x, ",", coord.z);

            const int d = std::max(std::abs(coord.x - c.x), std::abs(coord.z - c.z));
            if (d <= p.coreRadius)
                fail("cfg ", cfgIdx, ": painted inside core at d", d);
            // d > meshRadius = overhang: allowed, clipped by coverage in 3.2.7
        }

        for (int dz = -p.meshRadius; dz <= p.meshRadius; ++dz)
        {
            for (int dx = -p.meshRadius; dx <= p.meshRadius; ++dx)
            {
                const int d = std::max(std::abs(dx), std::abs(dz));
                if (d <= p.coreRadius) continue;

                if (!paint.contains({c.x + dx, c.z + dz}))
                    fail("cfg ", cfgIdx, ": hole at offset ", dx, ",", dz);
            }
        }

        // ---- proofs 2 + 4a/4b: quality floor + band membership ----------------------
        std::array<long long, 8> countAt {};

        for (const LodNodeKey& k : keys)
        {
            const int dNear = NearestDist(c, k.origin, k.footprint);
            const int inner = RingEdge(k.level - 1, p);

            if (dNear <= inner)
                fail("cfg ", cfgIdx, ": below inner band L", k.level, " f", k.footprint, " d", dNear);

            const int outer = k.level == p.maxLevel ? p.meshRadius
                            : k.footprint == 1      ? RingEdge(k.level, p)
                                                    : RingEdge(k.level, p) + 2 * k.footprint - 1;
            if (dNear > outer)
                fail("cfg ", cfgIdx, ": past outer band L", k.level, " f", k.footprint, " d", dNear);

            for (int dz = 0; dz < k.footprint; ++dz)
            {
                for (int dx = 0; dx < k.footprint; ++dx)
                {
                    const int d = std::max(std::abs(k.origin.x + dx - c.x),
                                           std::abs(k.origin.z + dz - c.z));
                    if (k.level > LevelForDistance(d, p))
                        fail("cfg ", cfgIdx, ": floor violated L", k.level, " at chunk d", d);
                }
            }

            if (k.footprint > 1) ++countAt[k.level];
        }

        // ---- proof 4c: per-level totals (loose blowup catch) ------------------------
        const auto ringArea = [](int _rIn, int _rOut) -> long long
        {
            if (_rOut <= _rIn) return 0;
            const long long a = 2LL * _rOut + 1, b = 2LL * _rIn + 1;
            return a * a - b * b;
        };

        for (int level = p.nodingStart; level <= p.maxLevel; ++level)
        {
            const int f    = NodeFootprint(level, p);
            const int rIn  = RingEdge(level - 1, p);
            const int rOut = std::min(level < p.maxLevel ? RingEdge(level, p)
                                                         : p.meshRadius, p.meshRadius);

            const long long hi = ringArea(rIn, std::min(rOut + 3 * f, p.meshRadius + f)) / (f * f);
            const long long lo = ringArea(rIn + 2 * f, rOut) / (f * f);

            if (countAt[level] < lo || countAt[level] > hi)
                fail("cfg ", cfgIdx, ": L", level, " count ", countAt[level],
                     " outside [", lo, ", ", hi, "]");
        }

        // ---- proof 5: hysteresis - a +-1 oscillation reaches a fixed point -----------
        {
            const Coord c1 { c.x + 1, c.z };

            const auto selectSet = [&](Coord _at, const NodeSet* _prev) -> NodeSet
            {
                std::vector<LodNodeKey> v;
                SelectNodes(_at, p, _prev, v);
                return { v.begin(), v.end() };
            };

            // membership near the window edges legitimately churns on a +-1 walk
            const auto isFringe = [&](const LodNodeKey& _k) -> bool
            {
                const int d = std::min(NearestDist(c,  _k.origin, _k.footprint),
                                       NearestDist(c1, _k.origin, _k.footprint));
                return d <= p.coreRadius + 1 || d >= p.meshRadius - 1;
            };

            const auto stableEq = [&](const NodeSet& _a, const NodeSet& _b) -> bool
            {
                for (const LodNodeKey& k : _a) if (!_b.contains(k) && !isFringe(k)) return false;
                for (const LodNodeKey& k : _b) if (!_a.contains(k) && !isFringe(k)) return false;
                return true;
            };

            const NodeSet a  = selectSet(c,  nullptr);   // fresh at c
            const NodeSet f0 = selectSet(c1, nullptr);   // fresh at c+1 (control)
            const NodeSet b  = selectSet(c1, &a);        // walk out: transitions happen
            const NodeSet d  = selectSet(c,  &b);        // walk back: deadband must hold
            const NodeSet e  = selectSet(c1, &d);        // out again: still held

            if (stableEq(a, f0)) fail("cfg ", cfgIdx, ": hysteresis control vacuous - fresh sets equal");
            if (!stableEq(b, d)) fail("cfg ", cfgIdx, ": deadband failed on the return walk");
            if (!stableEq(d, e)) fail("cfg ", cfgIdx, ": oscillation did not reach a fixed point");
        }
    }

    if (allOk)
        Log("[SUCCESS] [NodeSelect] partition/floor/bands/counts/negatives/hysteresis/determinism green (",
            cfgs.size(), " configs)");
    else
        Error("[NodeSelect] FAILED - see errors above (first 10 shown)");

    return allOk;
}

}
