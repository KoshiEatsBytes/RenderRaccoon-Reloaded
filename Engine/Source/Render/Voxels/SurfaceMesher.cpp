
#include "SurfaceMesher.h"
#include "Render/Voxels/ChunkMesher.h"

namespace RR
{
    using namespace CHUNK;
    using uInt32 = std::uint32_t;

    MeshData MeshSurface(int _dim, int _level,
        const std::vector<int> &_height,
        const std::vector<BLOCK> &_block,
        int _skirtDepth)
    {
        MeshData out;
        out.layout = VoxelVertexLayout();

        const int stride = 1 << _level;
        const int upFace = static_cast<int>(FACE::UP);
        uInt32    base = 0;

        // 4 verts per quad, not shared
        for (int sz = 0; sz < _dim - 1; ++sz)
        {
            for (int sx = 0; sx < _dim - 1; ++sx)
            {
                const int cx[4] = {sx, sx + 1, sx + 1, sx};
                const int cz[4] = {sx + 1, sz + 1, sz, sz};

                const float layer = static_cast<float>(
                    GetBlockInfo(_block[sx + sz * _dim]).faceLayer[upFace]);

                for (int corner = 0; corner < 4; ++corner)
                {
                    const int   gx = cx[corner];
                    const int   gz = cz[corner];
                    const float lx = static_cast<float>(gx * stride);
                    const float lz = static_cast<float>(gz * stride);
                    const float ly = static_cast<float>(_height[gx + gz * _dim] + 1);

                    // pos, local tile space
                    out.vertices.push_back(lx);
                    out.vertices.push_back(ly);
                    out.vertices.push_back(lz);
                    // normal facing up, top face
                    out.vertices.push_back(0.0f);
                    out.vertices.push_back(1.0f);
                    out.vertices.push_back(0.0f);
                    // uv, continuos per blocc
                    out.vertices.push_back(lx);
                    out.vertices.push_back(lz);
                    // layer
                    out.vertices.push_back(layer);
                }

                // mapping per face
                out.indices.push_back(base + 0);
                out.indices.push_back(base + 1);
                out.indices.push_back(base + 2);
                out.indices.push_back(base + 0);
                out.indices.push_back(base + 2);
                out.indices.push_back(base + 3);
                base += 4;
            }
        }

        if (_skirtDepth > 0)
        {
            const int sideFace = static_cast<int>(FACE::NORTH);

            // Renders a chunk's skirt given the grid pos
            auto emitSkirt = [&](int _gx0, int _gz0, int _gx1, int _gz1)
            {
                const float x0 = static_cast<float>(_gx0 * stride);
                const float z0 = static_cast<float>(_gz0 * stride);
                const float x1 = static_cast<float>(_gx1 * stride);
                const float z1 = static_cast<float>(_gz1 * stride);

                const float yT0 = static_cast<float>(_height[_gx0 + _gz0 * _dim] + 1);
                const float yT1 = static_cast<float>(_height[_gx1 + _gz1 * _dim] + 1);

                const float yB0 = yT0 - static_cast<float>(_skirtDepth);
                const float yB1 = yT1 - static_cast<float>(_skirtDepth);

                const float layer = static_cast<float>(
                    GetBlockInfo(_block[_gx0 + _gz0 * _dim]).faceLayer[sideFace]);

                const float px[4] = { x0, x1, x1, x0 };
                const float pz[4] = { z0, z1, z1, z0 };
                const float py[4] = { yT0, yT1, yB1, yB0 };
                const float uu[4] = { 0.f, static_cast<float>(stride), static_cast<float>(stride), 0.f};
                const float vv[4] = { 0.f, 0.f, static_cast<float>(stride), static_cast<float>(stride)};

                for (int k = 0; k < 4; ++k)
                {
                    // pos
                    out.vertices.push_back(px[k]);
                    out.vertices.push_back(py[k]);
                    out.vertices.push_back(pz[k]);
                    // nor
                    out.vertices.push_back(0.f);
                    out.vertices.push_back(1.f);
                    out.vertices.push_back(0.f);
                    // uv
                    out.vertices.push_back(uu[k]);
                    out.vertices.push_back(vv[k]);
                    // layer
                    out.vertices.push_back(layer);
                }

                // double sided face
                out.indices.push_back(base+0);
                out.indices.push_back(base+1);
                out.indices.push_back(base+2);
                out.indices.push_back(base+0);
                out.indices.push_back(base+2);
                out.indices.push_back(base+3);
                out.indices.push_back(base+0);
                out.indices.push_back(base+2);
                out.indices.push_back(base+1);
                out.indices.push_back(base+0);
                out.indices.push_back(base+3);
                out.indices.push_back(base+2);
                base += 4;
            };

            for (int s = 0; s < _dim - 1; ++s)
            {
                // north
                emitSkirt(s, 0, s + 1, 0);
                // south
                emitSkirt(s, _dim - 1, s + 1, _dim - 1);
                // west
                emitSkirt(0, s, 0, s + 1);
                // east
                emitSkirt(_dim - 1, s, _dim - 1, s + 1);
            }
        }

        return out;
    }
}










































