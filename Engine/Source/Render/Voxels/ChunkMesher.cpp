
#include "ChunkMesher.h"

namespace RR
{
    using namespace CHUNK;

    // FACE GEOMETRY TABLES --------------------------------------------------------------------------------------------

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

    // helper to check if a block is of a solid type
    static bool IsSolid(BlockId _id)
    {
        return GetBlockInfo(_id).solid;
    }

    // Is the neighbor cell solid?
    static bool NeighbourSolid(const Chunk& _chunk, const ChunkBorders& _borders, int _nx, int _ny, int _nz)
    {
        // always air above the world
        if (_ny >= kSizeY) return false;
        // FOR NOW no drawing below the world
        if (_ny < 0) return true;

        // check West if negative x
        if (_nx < 0)
            return IsSolid(_borders.GetBorderVoxel(ChunkBorders::BORDER::WEST, _nx, _ny, _nz));
        // East if positive x
        if (_nx >= kSizeX)
            return IsSolid(_borders.GetBorderVoxel(ChunkBorders::BORDER::EAST, _nx, _ny, _nz));
        // North if negative Z
        if (_nz < 0)
            return IsSolid(_borders.GetBorderVoxel(ChunkBorders::BORDER::NORTH, _nx, _ny, _nz));
        // South if positive z
        if (_nz >= kSizeZ)
            return IsSolid(_borders.GetBorderVoxel(ChunkBorders::BORDER::SOUTH, _nx, _ny, _nz));

        // If an interior cell has been requested
        return IsSolid(_chunk.At(_nx, _ny, _nz));
    }

    // MESH AND CHUNK VERTEX -------------------------------------------------------------------------------------------

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

    MeshData MeshChunk(const Chunk &_chunk, const ChunkBorders &_borders)
    {
        MeshData chunkMesh;
        chunkMesh.layout = VoxelVertexLayout();

        std::uint32_t baseVert = 0;

        // Y fastest scan, pairs with continuos column voxels
        for (int z = 0; z < kSizeZ; z++)
        {
            for (int x = 0; x < kSizeX; x++)
            {
                for (int y = 0; y < kSizeY; y++)
                {
                    const BlockId id = _chunk.At(x, y, z);

                    // Skip if not solid
                    if (!IsSolid(id)) continue;

                    const BlockInfo& info = GetBlockInfo(id);

                    for (int face = 0; face < 6; ++face)
                    {
                        // Hidden face cull, dont render face if neighbour in that direction is solid
                        if (NeighbourSolid(_chunk, _borders,
                            x + kFaceOffset[face][0],
                            y + kFaceOffset[face][1],
                            z + kFaceOffset[face][2]))
                        {
                            continue;
                        }

                        const float layer = info.faceLayer[face];

                        // Applies deterministic rotation on top and bottom face
                        int rot = 0;
                        const bool rotatable = (face == static_cast<int>(FACE::UP)    ||
                                                face == static_cast<int>(FACE::DOWN)) &&
                                                IsTexRotatable(static_cast<BLOCKTEX>(info.faceLayer[face]));

                        if (rotatable)
                        {
                            const int wx = _chunk.coord.x * kSizeX + x;
                            const int wz = _chunk.coord.z * kSizeZ + z;
                            rot = FaceRotation(wx, wz);
                        }

                        // Build vertex array, each voxel face has 4 vertices
                        for (int corner = 0; corner < 4; corner++)
                        {
                            // Position
                            chunkMesh.vertices.push_back(static_cast<float>(x) + kVoxelCorner[face][corner][0]);
                            chunkMesh.vertices.push_back(static_cast<float>(y) + kVoxelCorner[face][corner][1]);
                            chunkMesh.vertices.push_back(static_cast<float>(z) + kVoxelCorner[face][corner][2]);
                            // Normal
                            chunkMesh.vertices.push_back(kFaceNormal[face][0]);
                            chunkMesh.vertices.push_back(kFaceNormal[face][1]);
                            chunkMesh.vertices.push_back(kFaceNormal[face][2]);
                            // UV
                            chunkMesh.vertices.push_back(kFaceUV[(corner + rot) & 3][0]);
                            chunkMesh.vertices.push_back(kFaceUV[(corner + rot) & 3][1]);

                            chunkMesh.vertices.push_back(layer);
                        }

                        // 2 triangles per face
                        chunkMesh.indices.push_back(baseVert + 0);
                        chunkMesh.indices.push_back(baseVert + 1);
                        chunkMesh.indices.push_back(baseVert + 2);
                        chunkMesh.indices.push_back(baseVert + 0);
                        chunkMesh.indices.push_back(baseVert + 2);
                        chunkMesh.indices.push_back(baseVert + 3);

                        baseVert += 4;
                    }
                }
            }
        }

        return chunkMesh;
    }
}



























