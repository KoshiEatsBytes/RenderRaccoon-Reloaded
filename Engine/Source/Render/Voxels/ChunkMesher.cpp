
#include "ChunkMesher.h"

namespace RR
{
    using namespace CHUNK;

    // FACE GEOMETRY TABLES --------------------------------------------------------------------------------------------

    // VOXELS

    // Step one cell along face index to check for neighbor block
    static constexpr int kFaceOffset[6][3] = {
        { 1, 0, 0}, {-1, 0, 0},
        { 0, 1, 0}, { 0,-1, 0},
        { 0, 0, 1}, { 0, 0,-1},
    };

    // Which side the face exposed to
    static constexpr float kFaceNormal[6][3] = {
        { 1, 0, 0}, {-1, 0, 0},
        { 0, 1, 0}, { 0,-1, 0},
        { 0, 0, 1}, { 0, 0,-1},
    };

    // Where vertices sit at for each voxel
    static constexpr float kVoxelCorner[6][4][3] = {
        {{1,0,1},{1,0,0},{1,1,0},{1,1,1}}, // +X  East
        {{0,0,0},{0,0,1},{0,1,1},{0,1,0}}, // −X  West
        {{0,1,1},{1,1,1},{1,1,0},{0,1,0}}, // +Y  Up
        {{0,0,0},{1,0,0},{1,0,1},{0,0,1}}, // −Y  Down
        {{0,0,1},{1,0,1},{1,1,1},{0,1,1}}, // +Z  South
        {{1,0,0},{0,0,0},{0,1,0},{1,1,0}}, // −Z  North
    };

    // UV paired with corners above
    static constexpr float kFaceUV[4][2] = {
        {0,1},{1,1},{1,0},{0,0}
    };

    // CROSS BLOCKS

    // Corss blocks are two quads interlacing, different coords
    static constexpr float kCrossCorner[2][4][3] = {
        {{0,0,0},{1,0,1},{1,1,1},{0,1,0}},  // plane A
        {{1,0,0},{0,0,1},{0,1,1},{1,1,0}},  // plane B
    };

    // different UVs
    static constexpr float kCrossUV[4][2] = {
        {0,1},{1,1},{1,0},{0,0}
    };

    // MESH AND CHUNK VERTEX -------------------------------------------------------------------------------------------

    // if a block is solid or occludes
    static bool Occludes(BlockId _id, bool _fancyLeaves)
    {
        const BlockInfo& info = GetBlockInfo(_id);

        // fancy leaves dont hide anything
        if (_fancyLeaves && info.kind == RENDERKIND::LEAF) return false;

        return info.solid;
    }

    // Is the neighbor cell solid?
    static bool NeighbourOccludes(const Chunk& _chunk, const ChunkBorders& _borders,
        int _nx, int _ny, int _nz, bool _fancyLeaves)
    {
        // always air above the world
        if (_ny >= kSizeY) return false;
        // FOR NOW no drawing below the world
        if (_ny < 0) return true;

        // check West if negative x
        if (_nx < 0)
            return Occludes(_borders.GetBorderVoxel(ChunkBorders::BORDER::WEST, _nx, _ny, _nz), _fancyLeaves);
        // East if positive x
        if (_nx >= kSizeX)
            return Occludes(_borders.GetBorderVoxel(ChunkBorders::BORDER::EAST, _nx, _ny, _nz), _fancyLeaves);
        // North if negative Z
        if (_nz < 0)
            return Occludes(_borders.GetBorderVoxel(ChunkBorders::BORDER::NORTH, _nx, _ny, _nz), _fancyLeaves);
        // South if positive z
        if (_nz >= kSizeZ)
            return Occludes(_borders.GetBorderVoxel(ChunkBorders::BORDER::SOUTH, _nx, _ny, _nz), _fancyLeaves);

        // If an interior cell has been requested
        return Occludes(_chunk.At(_nx, _ny, _nz), _fancyLeaves);
    }

    // Builds the vertex layout for a voxel
    VertexLayout VoxelVertexLayout()
    {
        VertexLayout layout;
        layout.elements.push_back({VoxelAttrib::Position, 3, GL_FLOAT, 0});
        layout.elements.push_back({VoxelAttrib::Normal,   3, GL_FLOAT, sizeof(float) * 3});
        layout.elements.push_back({VoxelAttrib::UV,       2, GL_FLOAT, sizeof(float) * 6});
        layout.elements.push_back({VoxelAttrib::Layer,    1, GL_FLOAT, sizeof(float) * 8});
        layout.stride = sizeof(float) * 9;
        return layout;
    }

    static void EmitCube(MeshData& _out, const Chunk& _chunk, const ChunkBorders& _borders,
                         int _x, int _y, int _z, const BlockInfo& _info,
                         std::uint32_t& _baseVert, bool _fancyLeaves)
    {
        for (int face = 0; face < 6; ++face)
        {
            // Hidden face cull, dont render face if neighbour in that direction is solid
            if (NeighbourOccludes(
                _chunk,
                _borders,
                _x + kFaceOffset[face][0],
                _y + kFaceOffset[face][1],
                _z + kFaceOffset[face][2],
                _fancyLeaves))
            {
                continue;
            }

            const float layer = _info.faceLayer[face];

            // Applies deterministic rotation on top and bottom face
            int rot = 0;
            const bool rotatable = (face == static_cast<int>(FACE::UP)    ||
                                    face == static_cast<int>(FACE::DOWN)) &&
                                   IsTexRotatable(static_cast<BLOCKTEX>(_info.faceLayer[face]));

            if (rotatable)
            {
                const int wx = _chunk.coord.x * kSizeX + _x;
                const int wz = _chunk.coord.z * kSizeZ + _z;
                rot = FaceRotation(wx, wz);
            }

            // Build vertex array, each voxel face has 4 vertices
            for (int corner = 0; corner < 4; corner++)
            {
                // Position
                _out.vertices.push_back(static_cast<float>(_x) + kVoxelCorner[face][corner][0]);
                _out.vertices.push_back(static_cast<float>(_y) + kVoxelCorner[face][corner][1]);
                _out.vertices.push_back(static_cast<float>(_z) + kVoxelCorner[face][corner][2]);
                // Normal
                _out.vertices.push_back(kFaceNormal[face][0]);
                _out.vertices.push_back(kFaceNormal[face][1]);
                _out.vertices.push_back(kFaceNormal[face][2]);
                // UV
                _out.vertices.push_back(kFaceUV[(corner + rot) & 3][0]);
                _out.vertices.push_back(kFaceUV[(corner + rot) & 3][1]);

                _out.vertices.push_back(layer);
            }

            // 2 triangles per face
            _out.indices.push_back(_baseVert + 0);
            _out.indices.push_back(_baseVert + 1);
            _out.indices.push_back(_baseVert + 2);
            _out.indices.push_back(_baseVert + 0);
            _out.indices.push_back(_baseVert + 2);
            _out.indices.push_back(_baseVert + 3);

            _baseVert += 4;
        }
    }

    void EmitCross(MeshData& _out, int _x, int _y, int _z,
                   const BlockInfo& _info, std::uint32_t& _baseVert)
    {
        const float layer = _info.faceLayer[0];

        for (int panel = 0; panel < 2; ++panel)
        {
            for (int corner = 0; corner < 4; ++corner)
            {
                // Position
                _out.vertices.push_back(_x + kCrossCorner[panel][corner][0]);
                _out.vertices.push_back(_y + kCrossCorner[panel][corner][1]);
                _out.vertices.push_back(_z + kCrossCorner[panel][corner][2]);
                // Normal
                _out.vertices.push_back(0.0f);
                _out.vertices.push_back(1.0f);
                _out.vertices.push_back(0.0f);
                //Uvs
                _out.vertices.push_back(kCrossUV[corner][0]);
                _out.vertices.push_back(kCrossUV[corner][1]);

                _out.vertices.push_back(layer);
            }

            // 4 traingles per face, double sided regardles of culling
            _out.indices.push_back(_baseVert + 0);
            _out.indices.push_back(_baseVert + 1);
            _out.indices.push_back(_baseVert + 2);
            _out.indices.push_back(_baseVert + 0);
            _out.indices.push_back(_baseVert + 2);
            _out.indices.push_back(_baseVert + 3);
            _out.indices.push_back(_baseVert + 0);
            _out.indices.push_back(_baseVert + 2);
            _out.indices.push_back(_baseVert + 1);
            _out.indices.push_back(_baseVert + 0);
            _out.indices.push_back(_baseVert + 3);
            _out.indices.push_back(_baseVert + 2);

            _baseVert += 4;
        }
    }

    ChunkMeshes MeshChunk(const Chunk& _chunk, const ChunkBorders& _borders, bool _fancyLeaves)
    {
        ChunkMeshes out;
        out.opaque.layout = VoxelVertexLayout();
        out.veg.layout    = VoxelVertexLayout();

        std::uint32_t opaqueBase     = 0;
        std::uint32_t vegetationBase = 0;

        // Y fastest scan, pairs with continuos column voxels
        for (int z = 0; z < kSizeZ; z++)
        {
            for (int x = 0; x < kSizeX; x++)
            {
                for (int y = 0; y < kSizeY; y++)
                {
                    const BlockInfo& info = GetBlockInfo(_chunk.At(x, y, z));

                    switch (info.kind)
                    {
                        case RENDERKIND::CUBE:
                            if (info.solid) {
                                EmitCube(out.opaque, _chunk, _borders, x, y, z, info, opaqueBase, _fancyLeaves);
                            }
                            break;

                        case RENDERKIND::CROSS:
                            EmitCross(out.veg, x, y, z, info, vegetationBase);

                            break;

                        case RENDERKIND::LEAF:
                            EmitCube(out.veg, _chunk, _borders, x, y, z, info, vegetationBase, false);
                            break;
                    }
                }
            }
        }

        return out;
    }
}



























