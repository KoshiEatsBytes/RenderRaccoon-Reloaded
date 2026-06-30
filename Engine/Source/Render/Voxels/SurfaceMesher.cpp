
#include "SurfaceMesher.h"
#include "Render/Voxels/ChunkMesher.h"

namespace RR
{
    using namespace CHUNK;
    using uInt32 = std::uint32_t;

    MeshData MeshSurface(int _dim, int _level,
                         const std::vector<int>& _height,
                         const std::vector<BLOCK>& _block,
                         int _skirtDepth)
    {
        MeshData out;
        out.layout = VoxelVertexLayout();

        const int stride   = 1 << _level;
        const int cells    = _dim - 1;
        const int upFace   = static_cast<int>(FACE::UP);
        const int sideFace = static_cast<int>(FACE::NORTH);
        uInt32    baseIndex     = 0;

        // Reserve memory upfront
        const int maxQuads = cells * cells * 5;
        out.vertices.reserve(maxQuads * 4 * 9);
        out.indices.reserve(maxQuads * 6);

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
                const int   height    = _height[cx + cz * _dim];
                const BLOCK block     = _block[cx + cz * _dim];

                // Lookup texture layers
                const auto topLayer  = static_cast<float>(GetBlockInfo(block).faceLayer[upFace]);
                const auto sideLayer = static_cast<float>(GetBlockInfo(block).faceLayer[sideFace]);

                // world space of this cell
                const auto minX   = static_cast<float>(cx * stride);
                const auto maxX   = static_cast<float>((cx + 1) * stride);
                const auto minZ   = static_cast<float>(cz * stride);
                const auto maxZ   = static_cast<float>((cz + 1) * stride);
                const auto yTop = static_cast<float>(height + 1);

                // flat top face
                pushVertex(minX, yTop, maxZ, 0, 1, 0, minX, maxZ, topLayer);
                pushVertex(maxX, yTop, maxZ, 0, 1, 0, maxX, maxZ, topLayer);
                pushVertex(maxX, yTop, minZ, 0, 1, 0, maxX, minZ, topLayer);
                pushVertex(minX, yTop, minZ, 0, 1, 0, minX, minZ, topLayer);
                pushQuad();

                // get height of neighbor cell, drop a skirt if out of bounds
                auto neighbourHeight = [&](int _nx, int _nz) -> int
                {
                    if (_nx<0 || _nz<0 || _nx>=_dim || _nz>=_dim)
                    {
                        return height - _skirtDepth;
                    }

                    return _height[_nx + _nz*_dim];
                };

                auto pushRiser = [&](float _ax, float _az, float _uA,
                                     float _bx, float _bz, float _uB,
                                     float _nx, float _ny, float _nz,
                                     float _bottomY) -> void
                {
                    pushVertex(_ax, _bottomY, _az,  _nx, _ny, _nz,  _uA, _bottomY,  sideLayer);
                    pushVertex(_bx, _bottomY, _bz,  _nx, _ny, _nz,  _uB, _bottomY,  sideLayer);
                    pushVertex(_bx, yTop,    _bz,  _nx, _ny, _nz,  _uB, yTop,     sideLayer);
                    pushVertex(_ax, yTop,    _az,  _nx, _ny, _nz,  _uA, yTop,     sideLayer);
                    pushQuad();
                };

                // push a riser wherever the neighbour is lower. Off-tile neighbours
                // +X
                if (const int neighbour = neighbourHeight(cx + 1, cz); neighbour < height)
                {
                    pushRiser(maxX, maxZ, maxZ,  maxX, minZ, minZ,   1, 0,  0,
                        static_cast<float>(neighbour + 1));
                }
                // -X
                if (const int neighbour = neighbourHeight(cx - 1, cz); neighbour < height)
                {
                    pushRiser(minX, minZ, minZ,  minX, maxZ, maxZ,  -1, 0,  0,
                        static_cast<float>(neighbour + 1));
                }
                // +Z
                if (const int neighbour = neighbourHeight(cx, cz + 1); neighbour < height)
                {
                    pushRiser(minX, maxZ, minX,  maxX, maxZ, maxX,   0, 0,  1,
                        static_cast<float>(neighbour + 1));
                }
                // -Z
                if (const int neighbour = neighbourHeight(cx, cz - 1); neighbour < height)
                {
                    pushRiser(maxX, minZ, maxX,  minX, minZ, minX,   0, 0, -1,
                        static_cast<float>(neighbour + 1));
                }
            }
        }

        return out;
    }
}










































