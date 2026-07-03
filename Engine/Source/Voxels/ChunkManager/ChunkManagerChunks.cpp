
#include "ChunkManager.h"
#include "Render/Mesh.h"
#include "Helpers/Types.h"
#include "Voxels/Chunk.h"
#include "Voxels/ChunkData.h"
#include "Render/Voxels/ChunkMesher.h"
#include "Render/Voxels/SurfaceMesher.h"
#include "Voxels/LodNodeSelect.hpp"

namespace RR
{
    int ChunkManager::EnsureGenerated(CHUNK::Coord _centre)
    {
        const int chunkRange = m_lodEnabled ? m_coreRadius : m_meshRadius;
        int generated = 0;

        for (const CHUNK::Coord offset : m_genOffsets)
        {
            // discard if budget cap is overrun
            if (generated >= kGenBudget) break;

            // check if in range
            if (chessDist(offset) > chunkRange + 1) break;

            const int cx = _centre.x + offset.x;
            const int cz = _centre.z + offset.z;
            const CHUNK::Coord cords {cx, cz};

            if (!GetChunk(cords))
            {
                GenerateChunk(cords);
                ++generated;
            }
        }
        return generated;
    }

    int ChunkManager::EnsureMeshed(CHUNK::Coord _centre)
    {
        const int chunkRange = m_lodEnabled ? m_coreRadius : m_meshRadius;
        int meshed = 0;

        for (const CHUNK::Coord offset : m_genOffsets)
        {
            // discard if budget is overrun
            if (meshed >= kMeshBudget) break;

            // check if the chunk is in range
            if (chessDist(offset) > chunkRange) break;

            const int cx = _centre.x + offset.x;
            const int cz = _centre.z + offset.z;
            const CHUNK::Coord cords {cx, cz};
            Chunk* chunk = GetChunk(cords);

            if (!chunk || chunk->state != CHUNK::STATE::GENERATED) continue;
            if (!NeighboursGenerated(cords)) continue;

            BuildChunkMesh(*chunk);
            ++meshed;
        }
        return meshed;
    }

    void ChunkManager::GenerateChunk(CHUNK::Coord _coord)
    {
        auto chunk = std::make_unique<Chunk>(_coord);
        // Inject world gen
        m_generator(*chunk);
        chunk->state = CHUNK::STATE::GENERATED;
        m_chunks[_coord] = std::move(chunk);
    }

    void ChunkManager::BuildChunkMesh(Chunk& _chunk)
    {
        const ChunkBorders borders = GatherBorders(_chunk.coord);
        UploadChunkMesh(_chunk, MeshChunk(_chunk, borders, m_fancyLeaves));
    }

    void ChunkManager::UploadChunkMesh(Chunk& _chunk, ChunkMeshes&& _meshes)
    {
        // Make chunk mesh
        auto& chunkMesh = _meshes.opaque;
        _chunk.mesh = std::make_unique<Mesh>(chunkMesh.layout, chunkMesh.vertices, chunkMesh.indices);

        // Make Veg mesh
        auto& meshVeg = _meshes.veg;
        if (!meshVeg.Empty())
        {
            _chunk.vegMesh = std::make_unique<Mesh>(meshVeg.layout, meshVeg.vertices, meshVeg.indices);
        }

        _chunk.state = CHUNK::STATE::MESHED;
    }

    // Check neighboring chunks, if all generated return true
    bool ChunkManager::NeighboursGenerated(CHUNK::Coord _coord)
    {
        return GetChunk({_coord.x + 1, _coord.z}) &&
               GetChunk({_coord.x - 1, _coord.z}) &&
               GetChunk({_coord.x, _coord.z + 1}) &&
               GetChunk({_coord.x, _coord.z - 1});
    }

    // Returns chunk if present
    Chunk* ChunkManager::GetChunk(CHUNK::Coord _coord)
    {
        auto it = m_chunks.find(_coord);
        if (it != m_chunks.end())
        {
            return it->second.get();
        }

        return nullptr;
    }
    // Fetches chunk next to the given coords and returns their relevant borders
    ChunkBorders ChunkManager::GatherBorders(CHUNK::Coord _coord)
    {
        using namespace CHUNK;

        ChunkBorders borders {}; // all air default

        // EAST BORDERS
        if (Chunk* cEast = GetChunk({_coord.x + 1, _coord.z}))
        {
            for (int z = 0; z < kSizeZ; z++)
            {
                for (int y = 0; y < kSizeY; y++)
                {
                    borders.SetBorderVoxel(ChunkBorders::BORDER::EAST,
                        cEast->At(0, y, z), 0, y, z);
                }
            }
        }
        // WEST BORDERS
        if (Chunk* cWest = GetChunk({_coord.x - 1, _coord.z}))
        {
            for (int z = 0; z < kSizeZ; z++)
            {
                for (int y = 0; y < kSizeY; y++)
                {
                    borders.SetBorderVoxel(ChunkBorders::BORDER::WEST,
                        cWest->At(kSizeX - 1, y, z), 0, y, z);
                }
            }
        }
        // SOUTH BORDERS
        if (Chunk* cSouth = GetChunk({_coord.x, _coord.z + 1}))
        {
            for (int x = 0; x < kSizeX; x++)
            {
                for (int y = 0; y < kSizeY; y++)
                {
                    borders.SetBorderVoxel(ChunkBorders::BORDER::SOUTH,
                        cSouth->At(x, y, 0), x, y, 0);
                }
            }
        }
        // NORTH BORDERS
        if (Chunk* cNorth = GetChunk({_coord.x, _coord.z - 1}))
        {
            for (int x = 0; x < kSizeX; x++)
            {
                for (int y = 0; y < kSizeY; y++)
                {
                    borders.SetBorderVoxel(ChunkBorders::BORDER::NORTH,
                        cNorth->At(x, y, kSizeZ - 1), x, y, 0);
                }
            }
        }

        return borders;
    }

    // real chunk meshed at coord
    bool ChunkManager::ChunkReadyAt(CHUNK::Coord _coord) const
    {
        const auto it = m_chunks.find(_coord);
        return it != m_chunks.end() && it->second->state == CHUNK::STATE::MESHED;
    }

    bool ChunkManager::IsChunkMeshedAt(const vec3& _pos)
    {
        const Chunk* chunk = GetChunk(WorldToChunk(_pos));
        return chunk && chunk->state == CHUNK::STATE::MESHED;
    }
}