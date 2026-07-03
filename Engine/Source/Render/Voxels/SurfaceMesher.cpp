
#include "SurfaceMesher.h"
#include "Render/Voxels/ChunkMesher.h"

namespace RR
{
    using namespace CHUNK;
    using uInt32 = std::uint32_t;

    // one y band of a wall
    struct WallSlice
    {
        int   yMin;
        int   yMax;
        float layer;

        bool operator==(const WallSlice&) const = default;
    };

    MeshData MeshSurface(int _dim, int _level, int _bandMaxLevel, bool _greedy,
                     const std::vector<int>& _height,
                     const std::vector<BLOCK>& _block,
                     const std::vector<BLOCK>& _sideColumn)
    {
        MeshData out;
        out.layout = VoxelVertexLayout();

        const int stride      = 1 << _level;
        const int cells       = _dim - 1;
        const int upFace      = static_cast<int>(FACE::UP);
        const int sideFace    = static_cast<int>(FACE::NORTH);
        uInt32    baseIndex   = 0;

        const bool bandFaces = _level <= _bandMaxLevel;

        // sets band stepping
        int bandStep;
        if (_level == 3)
        {
            bandStep = kLodBandStepL3;
        }
        else if (_level == 2)
        {
            bandStep = kLodBandStepL2;
        }
        else
        {
            bandStep = kLodBandStepL1;
        }

        // Hint only, may be re-allocated for cliffs
        const int estQuads = cells * cells * 8;
        out.vertices.reserve(static_cast<std::size_t>(estQuads) * 4 * 9);
        out.indices.reserve (static_cast<std::size_t>(estQuads) * 12);

        // Pushes vertex layout into result
        auto pushVertex = [&](float _x,  float _y,  float _z,
                          float _nx, float _ny, float _nz,
                          float _u,  float _w,
                          float _layer)
        {
            out.vertices.insert(out.vertices.end(), {
            _x,  _y,   _z,
            _nx, _ny, _nz,
            _u,  _w,
            _layer
            });
        };

        // assembled quad from VL into output
        auto pushQuad = [&]()
        {
            out.indices.insert(out.indices.end(),
                {
                    // tris n1
                    baseIndex + 0,
                    baseIndex + 1,
                    baseIndex + 2,
                    // this n2
                    baseIndex + 0,
                    baseIndex + 2,
                    baseIndex + 3
                });
            baseIndex += 4;
        };

        auto pushQuadDouble = [&]()
        {
            // double faced quad
            out.indices.insert(out.indices.end(),
                {
                    // front
                    baseIndex + 0,
                    baseIndex + 1,
                    baseIndex + 2,
                    baseIndex + 0,
                    baseIndex + 2,
                    baseIndex + 3,
                    // back
                    baseIndex + 0,
                    baseIndex + 2,
                    baseIndex + 1,
                    baseIndex + 0,
                    baseIndex + 3,
                    baseIndex + 2
                });
            baseIndex += 4;
        };

        if (!_greedy)
        {
            // one quad per cell
            for (int cz = 1; cz < cells; ++cz)
            {
                for (int cx = 1; cx < cells; ++cx)
                {
                    const int   height   = _height[cx + cz * _dim];
                    const BLOCK block    = _block [cx + cz * _dim];
                    const auto  topLayer = static_cast<float>(GetBlockInfo(block).faceLayer[upFace]);

                    // world space of this cell
                    const auto minX = static_cast<float>((cx - 1) * stride);
                    const auto maxX = static_cast<float>(cx * stride);
                    const auto minZ = static_cast<float>((cz - 1) * stride);
                    const auto maxZ = static_cast<float>(cz * stride);
                    const auto yTop = static_cast<float>(height + 1);

                    // flat top face
                    pushVertex(minX, yTop, maxZ, 0, 1, 0, minX, maxZ, topLayer);
                    pushVertex(maxX, yTop, maxZ, 0, 1, 0, maxX, maxZ, topLayer);
                    pushVertex(maxX, yTop, minZ, 0, 1, 0, maxX, minZ, topLayer);
                    pushVertex(minX, yTop, minZ, 0, 1, 0, minX, minZ, topLayer);
                    pushQuad();
                }
            }
        }
        else
        {
            // Greedy rectable cover over the rendered grid
            const int n = cells - 1;
            std::vector<std::uint8_t> used(static_cast<std::size_t>(n) * n, 0);

            // two cells merge if same surface height and blocc
            auto sameKey = [&](int _cxA, int _czA, int _cxB, int _czB)
            {
                return _height[_cxA + _czA * _dim] == _height[_cxB + _czB * _dim] &&
                       _block [_cxA + _czA * _dim] == _block [_cxB + _czB * _dim];
            };

            for (int gz = 0; gz < n; ++gz)
            {
                for (int gx = 0; gx < n; ++gx)
                {
                    if (used[gx + gz * n]) continue;

                    const int cx = gx + 1;
                    const int cz = gz + 1;

                    // grow along X
                    int rectW = 1;

                    while (gx + rectW < n && !used[(gx + rectW) + gz * n] &&
                           sameKey(cx, cz, cx + rectW, cz))
                    {
                        rectW++;
                    }

                    // grow along Z
                    int rectH = 1;

                    while (gz + rectH < n)
                    {
                        bool rowMatches = true;

                        for (int i = 0; i < rectW; ++i)
                        {
                            // check if they match, if not stop elongating
                            if (used[(gx + i) + (gz + rectH) * n] ||
                                !sameKey(cx, cz, cx + i, cz + rectH))
                            {
                                rowMatches = false;
                                break;
                            }
                        }

                        if (!rowMatches) break;
                        ++rectH;
                    }

                    // claim the created rect
                    for (int dz = 0; dz < rectH; ++dz)
                    {
                        for (int dx = 0; dx < rectW; ++dx)
                        {
                            used[(gx + dx) + (gz + dz) * n] = 1;
                        }
                    }

                    // Emit one quad covering the globbed rect
                    const int   height   = _height[cx + cz * _dim];
                    const BLOCK block    = _block [cx + cz * _dim];
                    const auto  topLayer = static_cast<float>(GetBlockInfo(block).faceLayer[upFace]);

                    const auto minX = static_cast<float>((cx - 1) * stride);
                    const auto maxX = static_cast<float>((cx - 1 + rectW) * stride);
                    const auto minZ = static_cast<float>((cz - 1) * stride);
                    const auto maxZ = static_cast<float>((cz - 1 + rectH) * stride);
                    const auto yTop = static_cast<float>(height + 1);

                    // assemble vertices and push rect
                    pushVertex(minX, yTop, maxZ, 0, 1, 0, minX, maxZ, topLayer);
                    pushVertex(maxX, yTop, maxZ, 0, 1, 0, maxX, maxZ, topLayer);
                    pushVertex(maxX, yTop, minZ, 0, 1, 0, maxX, minZ, topLayer);
                    pushVertex(minX, yTop, minZ, 0, 1, 0, minX, minZ, topLayer);
                    pushQuad();
                }
            }
        }

        // resue per edge
        std::vector<WallSlice> slices;

        // side strata for cell at height y
        auto sideLayerAt = [&](int _cx, int _cz, int _y) -> float
        {
            const int height  = _height[_cx + _cz * _dim];
            const int colBase = (_cx + _cz * _dim) * kLodBandDepth;
            const int depth   = std::clamp(height - _y, 0, kLodBandDepth - 1);

            return GetBlockInfo(_sideColumn[colBase + depth]).faceLayer[sideFace];
        };

        // slice stack for the wall of cell against neighbour top
        auto buildSlices = [&](int _cx, int _cz, int _neighbourTop,
                               std::vector<WallSlice>& _out)
        {
            _out.clear();
            const int buildHeight = _height[_cx + _cz * _dim];

            // no wall on this edge
            if (_neighbourTop >= buildHeight) return;

            const int bottomY = _neighbourTop + 1;

            // skirt or far ring
            if (!bandFaces)
            {
                const int capBands  = stride;
                const int capBottom = std::max(bottomY, buildHeight + 1 - capBands);

                if (capBottom > bottomY)
                {
                    _out.push_back({
                        bottomY, capBottom, sideLayerAt(_cx, _cz, buildHeight - 1)
                    });
                }

                _out.push_back({
                    capBottom, buildHeight + 1, sideLayerAt(_cx, _cz, buildHeight)
                });

                return;
            }

            // banded cliff face
            for (int y = bottomY; y <= buildHeight; y += bandStep)
            {
                const int topBlock = std::min(y + bandStep - 1, buildHeight);

                _out.push_back({
                    y, std::min(y + bandStep, buildHeight + 1),
                         sideLayerAt(_cx, _cz, topBlock) });
            }
        };

        // emit one slice stack along an edge
        auto emitSlices = [&](const std::vector<WallSlice>& _slices,
                              float _ax, float _az, float _uA,
                              float _bx, float _bz, float _uB,
                              float _nx, float _ny, float _nz)
        {
            for (const WallSlice& slice : _slices)
            {
                const auto lo = static_cast<float>(slice.yMin);
                const auto hi = static_cast<float>(slice.yMax);

                // double sided quad
                pushVertex(_ax, lo, _az, _nx, _ny, _nz, _uA, lo, slice.layer);
                pushVertex(_bx, lo, _bz, _nx, _ny, _nz, _uB, lo, slice.layer);
                pushVertex(_bx, hi, _bz, _nx, _ny, _nz, _uB, hi, slice.layer);
                pushVertex(_ax, hi, _az, _nx, _ny, _nz, _uA, hi, slice.layer);
                pushQuadDouble();
            }
        };

        if (!_greedy)
        {
            // wall pass for cells
            for (int cz = 1; cz < cells; ++cz)
            {
                for (int cx = 1; cx < cells; ++cx)
                {
                    // world space of this cell
                    const auto minX = static_cast<float>((cx - 1) * stride);
                    const auto maxX = static_cast<float>(cx * stride);
                    const auto minZ = static_cast<float>((cz - 1) * stride);
                    const auto maxZ = static_cast<float>(cz * stride);

                    // West
                    buildSlices(cx, cz, _height[(cx + 1) + cz * _dim], slices);
                    emitSlices(slices, maxX, maxZ, maxZ,  maxX, minZ, minZ,   1, 0, 0);
                    // East
                    buildSlices(cx, cz, _height[(cx - 1) + cz * _dim], slices);
                    emitSlices(slices, minX, minZ, minZ,  minX, maxZ, maxZ,  -1, 0, 0);
                    // South
                    buildSlices(cx, cz, _height[cx + (cz + 1) * _dim], slices);
                    emitSlices(slices, minX, maxZ, minX,  maxX, maxZ, maxX,   0, 0, 1);
                    // North
                    buildSlices(cx, cz, _height[cx + (cz - 1) * _dim], slices);
                    emitSlices(slices, maxX, minZ, maxX,  minX, minZ, minX,   0, 0, -1);
                }
            }
        }
        else
        {
            // per direction run merge
            // identical slices stack along the tangent
            constexpr int offX[4] = { 1, -1, 0,  0 };
            constexpr int offZ[4] = { 0,  0, 1, -1 };

            // stack shared by current run
            std::vector<WallSlice> run;
            std::vector<WallSlice> next;

            // emit finished run
            auto flushRun = [&](int _dir, int _line, int _tMin, int _tMax)
            {
                const auto lineMin = static_cast<float>((_line - 1) * stride);
                const auto lineMax = static_cast<float>(_line * stride);
                const auto tMin    = static_cast<float>((_tMin - 1) * stride);
                const auto tMax    = static_cast<float>(_tMax * stride);

                switch (_dir)
                {
                    // West
                    case 0:
                        emitSlices(run, lineMax, tMax, tMax,
                                        lineMax, tMin, tMin,
                                        1, 0,  0);
                        break;

                    // East
                    case 1:
                        emitSlices(run, lineMin, tMin, tMin,
                                        lineMin, tMax, tMax,
                                        -1, 0,  0);
                        break;

                    // South
                    case 2:
                        emitSlices(run, tMin, lineMax, tMin,
                                        tMax, lineMax, tMax,
                                        0, 0,  1);
                        break;

                    // North
                    default:
                        emitSlices(run, tMax, lineMin, tMax,
                                        tMin, lineMin, tMin,
                                        0, 0, -1);
                        break;
                }
            };

            for (int dir = 0; dir < 4; ++dir)
            {
                for (int line = 1; line < cells; ++line)
                {
                    int runStart = -1;
                    run.clear();

                    for (int t = 1; t < cells; ++t)
                    {
                        // adjusted bi direction
                        const int cx = dir < 2 ? line : t;
                        const int cz = dir < 2 ? t : line;

                        buildSlices(cx, cz,_height[(cx + offX[dir]) + (cz + offZ[dir]) * _dim], next);

                        // identical stack, run keep growing
                        if (runStart != -1 && next == run) continue;

                        // stack changed, flush, start new run
                        if (runStart != -1 && !run.empty())
                        {
                            flushRun(dir, line, runStart, t - 1);
                        }

                        run.swap(next);
                        runStart = t;
                    }

                    // tail run of line
                    if (!run.empty())
                    {
                        flushRun(dir, line, runStart, cells - 1);
                    }
                }
            }
        }

        return out;
    }

    MeshData MeshProxies(const std::vector<LodTreeProxy> &_trees, int _level)
    {
        MeshData out;
        out.layout = VoxelVertexLayout();

        const int upFace    = static_cast<int>(FACE::UP);
        const int sideFace  = static_cast<int>(FACE::NORTH);
        uInt32    baseIndex = 0;

        // up to 9 faces/tree 
        out.vertices.reserve(_trees.size() * 9 * 4 * 9);
        out.indices.reserve (_trees.size() * 9 * 6);

        // Pushes vertex layout into result
        auto pushVertex = [&](float _x,  float _y,  float _z,
                          float _nx, float _ny, float _nz,
                          float _u,  float _w,
                          float _layer)
        {
            out.vertices.insert(out.vertices.end(), {
            _x,  _y,   _z,
            _nx, _ny, _nz,
            _u,  _w,
            _layer
            });
        };

        // assembled quad from VL into output
        auto pushQuad = [&]()
        {
            out.indices.insert(out.indices.end(),
                {
                    // tris n1
                    baseIndex + 0,
                    baseIndex + 1,
                    baseIndex + 2,
                    // this n2
                    baseIndex + 0,
                    baseIndex + 2,
                    baseIndex + 3
                });
            baseIndex += 4;
        };

        // box witrh optional top for proxies
        auto pushBox = [&](float _x0, float _x1, float _y0, float _y1, float _z0, float _z1,
                           float _layer, bool _top)
        {
            const float width  = _x1 - _x0;
            const float height = _y1 - _y0;
            const float depth  = _z1 - _z0;

            if (_top)
            {
                pushVertex(_x0,_y1,_z1, 0,1,0, 0,depth, _layer);
                pushVertex(_x1,_y1,_z1, 0,1,0, width,depth, _layer);
                pushVertex(_x1,_y1,_z0, 0,1,0, width,0, _layer);
                pushVertex(_x0,_y1,_z0, 0,1,0, 0,0, _layer);
                pushQuad();
            }
            // West
            pushVertex(_x1,_y0,_z1, 1,0,0, 0,0, _layer);
            pushVertex(_x1,_y0,_z0, 1,0,0, depth,0, _layer);
            pushVertex(_x1,_y1,_z0, 1,0,0, depth,height, _layer);
            pushVertex(_x1,_y1,_z1, 1,0,0, 0,height, _layer);
            pushQuad();
            // East
            pushVertex(_x0,_y0,_z0, -1,0,0, 0,0, _layer);
            pushVertex(_x0,_y0,_z1, -1,0,0, depth,0, _layer);
            pushVertex(_x0,_y1,_z1, -1,0,0, depth,height, _layer);
            pushVertex(_x0,_y1,_z0, -1,0,0, 0,height, _layer);
            pushQuad();
            // South
            pushVertex(_x0,_y0,_z1, 0,0,1, 0,0, _layer);
            pushVertex(_x1,_y0,_z1, 0,0,1, width,0, _layer);
            pushVertex(_x1,_y1,_z1, 0,0,1, width,height, _layer);
            pushVertex(_x0,_y1,_z1, 0,0,1, 0,height, _layer);
            pushQuad();
            // North
            pushVertex(_x1,_y0,_z0, 0,0,-1, 0,0, _layer);
            pushVertex(_x0,_y0,_z0, 0,0,-1, width,0, _layer);
            pushVertex(_x0,_y1,_z0, 0,0,-1, width,height, _layer);
            pushVertex(_x1,_y1,_z0, 0,0,-1, 0,height, _layer);
            pushQuad();
        };

        // sqwaure base and smaller top, tryin to mimic tree
        auto pushCrown = [&](float _cx, float _cz, float _baseR, float _topR,
                             float _y0, float _y1, float _layer)
        {
            const float bx0 = _cx - _baseR,         bx1 = _cx + _baseR + 1.0f;
            const float bz0 = _cz - _baseR,         bz1 = _cz + _baseR + 1.0f;
            const float ctrX = (bx0 + bx1) * 0.5f,  ctrZ = (bz0 + bz1) * 0.5f;
            const float tx0 = ctrX - _topR,         tx1 = ctrX + _topR;
            const float tz0 = ctrZ - _topR,         tz1 = ctrZ + _topR;

            const float width  = bx1 - bx0;
            const float height = _y1 - _y0;
            const float depth  = bz1 - bz0;

            // top
            pushVertex(tx0,_y1,tz1, 0,1,0, 0,depth, _layer);
            pushVertex(tx1,_y1,tz1, 0,1,0, width,depth, _layer);
            pushVertex(tx1,_y1,tz0, 0,1,0, width,0, _layer);
            pushVertex(tx0,_y1,tz0, 0,1,0, 0,0, _layer);
            pushQuad();
            // West
            pushVertex(bx1,_y0,bz1, 1,0,0, 0,0, _layer);
            pushVertex(bx1,_y0,bz0, 1,0,0, depth,0, _layer);
            pushVertex(tx1,_y1,tz0, 1,0,0, depth,height, _layer);
            pushVertex(tx1,_y1,tz1, 1,0,0, 0,height, _layer);
            pushQuad();
            // East
            pushVertex(bx0,_y0,bz0, -1,0,0, 0,0, _layer);
            pushVertex(bx0,_y0,bz1, -1,0,0, depth,0, _layer);
            pushVertex(tx0,_y1,tz1, -1,0,0, depth,height, _layer);
            pushVertex(tx0,_y1,tz0, -1,0,0, 0,height, _layer);
            pushQuad();
            // South
            pushVertex(bx0,_y0,bz1, 0,0,1, 0,0, _layer);
            pushVertex(bx1,_y0,bz1, 0,0,1, width,0, _layer);
            pushVertex(tx1,_y1,tz1, 0,0,1, width,height, _layer);
            pushVertex(tx0,_y1,tz1, 0,0,1, 0,height, _layer);
            pushQuad();
            // North
            pushVertex(bx1,_y0,bz0, 0,0,-1, 0,0, _layer);
            pushVertex(bx0,_y0,bz0, 0,0,-1, width,0, _layer);
            pushVertex(tx0,_y1,tz0, 0,0,-1, width,height, _layer);
            pushVertex(tx1,_y1,tz0, 0,0,-1, 0,height, _layer);
            pushQuad();
        };

        for (const LodTreeProxy& tree : _trees)
        {
            // canopy is a UniformBlock, log is sided fatch what i sneede
            const auto canopyLayer = static_cast<float>(GetBlockInfo(tree.canopy).faceLayer[upFace]);
            const auto logLayer    = static_cast<float>(GetBlockInfo(tree.log).faceLayer[sideFace]);

            const auto cx     = static_cast<float>(tree.localX);
            const auto cz     = static_cast<float>(tree.localZ);
            const auto radius = static_cast<float>(tree.radius);

            const auto trunkBot  = static_cast<float>(tree.baseY + 1);
            const auto trunkTop  = trunkBot + static_cast<float>(tree.trunkHeight);
            const auto canopyTop = trunkTop + static_cast<float>(tree.canopyHeight);

            // trunk
            if (tree.trunkHeight > 0)
            {
                pushBox(cx, cx + 1.0f, trunkBot, trunkTop, cz, cz + 1.0f, logLayer, tree.canopyHeight == 0);
            }
                
            // crown for trees only
            if (tree.canopyHeight > 0)
            {
                const float topR = (radius + 0.5f) * tree.crownTopFrac;
                pushCrown(cx, cz, radius, topR, trunkTop, canopyTop, canopyLayer);
            }
        }

        return out;
    }
}










































