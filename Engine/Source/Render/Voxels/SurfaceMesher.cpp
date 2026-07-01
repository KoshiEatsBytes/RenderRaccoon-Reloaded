
#include "SurfaceMesher.h"
#include "Render/Voxels/ChunkMesher.h"

namespace RR
{
    using namespace CHUNK;
    using uInt32 = std::uint32_t;

    MeshData MeshSurface(int _dim, int _level,
                     const std::vector<int>& _height,
                     const std::vector<BLOCK>& _block,
                     const std::vector<BLOCK>& _sideColumn)
    {
        MeshData out;
        out.layout = VoxelVertexLayout();

        const bool bandFaces = _level <= 1;
        const int stride      = 1 << _level;
        const int cells       = _dim - 1;
        const int upFace      = static_cast<int>(FACE::UP);
        const int sideFace    = static_cast<int>(FACE::NORTH);
        uInt32    baseIndex   = 0;

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

        // Iterate inner cells in heightmap grid
        for (int cz = 1; cz < cells; ++cz)
        {
            for (int cx = 1; cx < cells; ++cx)
            {
                const int   height = _height[cx + cz * _dim];
                const BLOCK block  = _block [cx + cz * _dim];

                // Lookup texture layers
                const auto topLayer = static_cast<float>(GetBlockInfo(block).faceLayer[upFace]);

                // world space of this cell
                const auto minX = static_cast<float>((cx - 1) * stride);
                const auto maxX = static_cast<float>(cx * stride);
                const auto minZ = static_cast<float>((cz - 1) * stride);
                const auto maxZ = static_cast<float>(cz* stride);
                const auto yTop = static_cast<float>(height + 1);

                // flat top face
                pushVertex(minX, yTop, maxZ, 0, 1, 0, minX, maxZ, topLayer);
                pushVertex(maxX, yTop, maxZ, 0, 1, 0, maxX, maxZ, topLayer);
                pushVertex(maxX, yTop, minZ, 0, 1, 0, maxX, minZ, topLayer);
                pushVertex(minX, yTop, minZ, 0, 1, 0, minX, minZ, topLayer);
                pushQuad();

                // side blocks whos face are shown
                const int colBase = (cx + cz * _dim) * kLodBandDepth;
                auto sideLayerAt = [&](int _y) -> float
                {
                    const int   depth = std::clamp(height - _y, 0, kLodBandDepth - 1);
                    const BLOCK bLoc = _sideColumn[colBase + depth];

                    return GetBlockInfo(bLoc).faceLayer[sideFace];
                };

                // get height of neighbor cell, if false drop skirt
                auto neighbour = [&](int _nx, int _nz, int& _outTop)
                {
                    // unconditional apron
                    _outTop = _height[_nx + _nz * _dim];
                };

                // wall along one edge down to neighbour top height
                auto pushWall = [&](float _ax, float _az, float _uA,
                                    float _bx, float _bz, float _uB,
                                    float _nx, float _ny, float _nz,
                                    int _neighbourTop) -> void
                {
                    const int bottomY = _neighbourTop + 1;

                    // quad spanning a world y range up to this edge
                    auto pushQuadY = [&](float _lo, float _hi, float _layer)
                    {
                        pushVertex(_ax, _lo, _az, _nx,_ny,_nz, _uA, _lo, _layer);
                        pushVertex(_bx, _lo, _bz, _nx,_ny,_nz, _uB, _lo, _layer);
                        pushVertex(_bx, _hi, _bz, _nx,_ny,_nz, _uB, _hi, _layer);
                        pushVertex(_ax, _hi, _az, _nx,_ny,_nz, _uA, _hi, _layer);
                        pushQuadDouble();
                    };

                    // skirt or far ring
                    if (!bandFaces)
                    {
                        const int capBands  = stride;
                        const int capBottom = std::max(bottomY, height + 1 - capBands);

                        if (capBottom > bottomY)
                        {
                            pushQuadY(static_cast<float>(bottomY),
                                      static_cast<float>(capBottom),
                                         sideLayerAt(height - 1));
                        }

                        pushQuadY(static_cast<float>(capBottom), yTop, sideLayerAt(height));
                        return;
                    }

                    // banded cliff face
                    for (int y = bottomY; y <= height; y += kLodBandStep)
                    {
                        // sample top block, so topmost keeps at 0
                        const int   topBlock = std::min(y + kLodBandStep - 1, height);
                        const float layer    = sideLayerAt(topBlock);
                        const float y0       = static_cast<float>(y);
                        const float y1       = static_cast<float>(std::min(y + kLodBandStep, height + 1));

                        pushQuadY(y0, y1, layer);
                    }
                };

                int  nTop;

                // West
                neighbour(cx + 1, cz, nTop);
                if (nTop < height)
                {
                    pushWall(maxX, maxZ, maxZ,  maxX, minZ, minZ,   1, 0, 0, nTop);
                }
                // East
                neighbour(cx - 1, cz, nTop);
                if (nTop < height)
                {
                    pushWall(minX, minZ, minZ,  minX, maxZ, maxZ,  -1, 0, 0, nTop);
                }
                // South
                neighbour(cx, cz + 1, nTop);
                if (nTop < height)
                {
                    pushWall(minX, maxZ, minX,  maxX, maxZ, maxX,   0, 0, 1, nTop);
                }
                // North
                neighbour(cx, cz - 1, nTop);
                if (nTop < height)
                {
                    pushWall(maxX, minZ, maxX,  minX, minZ, minX,   0,0,-1, nTop);
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










































