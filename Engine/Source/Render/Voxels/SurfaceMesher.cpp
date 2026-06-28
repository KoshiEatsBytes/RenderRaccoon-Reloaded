
#include "SurfaceMesher.h"
#include "Render/Voxels/ChunkMesher.h"

namespace RR
{
    using namespace CHUNK;
    using uInt32 = std::uint32_t;

    MeshData MeshSurface(int _dim, int _level,
        const std::vector<int> &_height,
        const std::vector<BLOCK> &_block)
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

        return out;
    }
}
