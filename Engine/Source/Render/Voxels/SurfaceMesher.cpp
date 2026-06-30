
#include "SurfaceMesher.h"
#include "Render/Voxels/ChunkMesher.h"

namespace RR
{
    using namespace CHUNK;
    using uInt32 = std::uint32_t;

    MeshData MeshSurface(int _dim, int _level,
                     const std::vector<int>& _height,
                     const std::vector<BLOCK>& _block,
                     const std::vector<BLOCK>& _sideColumn,
                     int _skirtDepth)
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
        out.indices.reserve (static_cast<std::size_t>(estQuads) * 6);

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

        // Iterate cells in heightmap grid
        for (int cz = 0; cz < cells; ++cz)
        {
            for (int cx = 0; cx < cells; ++cx)
            {
                const int   height = _height[cx + cz * _dim];
                const BLOCK block  = _block [cx + cz * _dim];

                // Lookup texture layers
                const auto topLayer = static_cast<float>(GetBlockInfo(block).faceLayer[upFace]);

                // world space of this cell
                const auto minX = static_cast<float>(cx * stride);
                const auto maxX = static_cast<float>((cx + 1) * stride);
                const auto minZ = static_cast<float>(cz * stride);
                const auto maxZ = static_cast<float>((cz + 1) * stride);
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
                auto neighbour = [&](int _nx, int _nz, int& _outTop) -> bool
                {
                    if (_nx < 0 || _nz < 0 || _nx >= _dim || _nz >= _dim)
                    {
                        _outTop = height - _skirtDepth;
                        return false;
                    }

                    _outTop = _height[_nx + _nz * _dim];
                    return true;
                };

                // wall along one edge down to neighbour top height
                auto pushWall = [&](float _ax, float _az, float _uA,
                                    float _bx, float _bz, float _uB,
                                    float _nx, float _ny, float _nz,
                                    int _neighbourTop, bool _banded) -> void
                {
                    const int bottomY = _neighbourTop + 1;

                    // quad spanning a world y range up to this edge
                    auto pushQuadY = [&](float _lo, float _hi, float _layer)
                    {
                        pushVertex(_ax, _lo, _az, _nx,_ny,_nz, _uA, _lo, _layer);
                        pushVertex(_bx, _lo, _bz, _nx,_ny,_nz, _uB, _lo, _layer);
                        pushVertex(_bx, _hi, _bz, _nx,_ny,_nz, _uB, _hi, _layer);
                        pushVertex(_ax, _hi, _az, _nx,_ny,_nz, _uA, _hi, _layer);
                        pushQuad();
                    };

                    // skirt or far ring
                    if (!_banded || !bandFaces)
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

                    // L1: banded cliff face in kLodBandStep-tall steps
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
                bool inBetween = neighbour(cx + 1, cz, nTop);
                if (nTop < height)
                {
                    pushWall(maxX, maxZ, maxZ,  maxX, minZ, minZ,   1,0, 0, nTop, inBetween);
                }
                // East
                inBetween = neighbour(cx - 1, cz, nTop);
                if (nTop < height)
                {
                    pushWall(minX, minZ, minZ,  minX, maxZ, maxZ,  -1,0, 0, nTop, inBetween);
                }
                // South
                inBetween = neighbour(cx, cz + 1, nTop);
                if (nTop < height)
                {
                    pushWall(minX, maxZ, minX,  maxX, maxZ, maxX,   0,0, 1, nTop, inBetween);
                }
                // North
                inBetween = neighbour(cx, cz - 1, nTop);
                if (nTop < height)
                {
                    pushWall(maxX, minZ, maxX,  minX, minZ, minX,   0,0,-1, nTop, inBetween);
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

        // skip bottom
        out.vertices.reserve(_trees.size() * 5 * 4 * 9);
        out.indices.reserve (_trees.size() * 5 * 6);

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

        for (const LodTreeProxy& tree : _trees)
        {
            // canopy is a UniformBlock so side and layers match
            const auto top  = static_cast<float>(GetBlockInfo(tree.canopy).faceLayer[upFace]);
            const auto side = static_cast<float>(GetBlockInfo(tree.canopy).faceLayer[sideFace]);

            const float x0 = static_cast<float>(tree.localX);
            const float x1 = static_cast<float>(tree.localX + kProxyWidth);
            const float z0 = static_cast<float>(tree.localZ);
            const float z1 = static_cast<float>(tree.localZ + kProxyWidth);
            const float y0 = static_cast<float>(tree.baseY + 1);
            const float y1 = static_cast<float>(tree.baseY + 1 + tree.height);
            const float fw = static_cast<float>(kProxyWidth);
            const float fh = static_cast<float>(tree.height);

            // TOP
            pushVertex(x0,y1,z1, 0,1,0, 0,fw,  top);
            pushVertex(x1,y1,z1, 0,1,0, fw,fw, top);
            pushVertex(x1,y1,z0, 0,1,0, fw,0,  top);
            pushVertex(x0,y1,z0, 0,1,0, 0,0,   top);
            pushQuad();

            // West
            pushVertex(x1,y0,z1, 1,0,0, 0,0,   side);
            pushVertex(x1,y0,z0, 1,0,0, fw,0,  side);
            pushVertex(x1,y1,z0, 1,0,0, fw,fh, side);
            pushVertex(x1,y1,z1, 1,0,0, 0,fh,  side);
            pushQuad();

            // East
            pushVertex(x0,y0,z0, -1,0,0, 0,0,   side);
            pushVertex(x0,y0,z1, -1,0,0, fw,0,  side);
            pushVertex(x0,y1,z1, -1,0,0, fw,fh, side);
            pushVertex(x0,y1,z0, -1,0,0, 0,fh,  side);
            pushQuad();

            // South
            pushVertex(x0,y0,z1, 0,0,1, 0,0,   side);
            pushVertex(x1,y0,z1, 0,0,1, fw,0,  side);
            pushVertex(x1,y1,z1, 0,0,1, fw,fh, side);
            pushVertex(x0,y1,z1, 0,0,1, 0,fh,  side);
            pushQuad();

            // North
            pushVertex(x1,y0,z0, 0,0,-1, 0,0,   side);
            pushVertex(x0,y0,z0, 0,0,-1, fw,0,  side);
            pushVertex(x0,y1,z0, 0,0,-1, fw,fh, side);
            pushVertex(x1,y1,z0, 0,0,-1, 0,fh,  side);
            pushQuad();
        }

        return out;
    }
}










































