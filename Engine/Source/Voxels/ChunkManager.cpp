
#include "ChunkManager.h"

#include <cmath>
#include <algorithm>

#include "Render/Mesh.h"
#include "Render/RenderQueue.h"
#include "Render/Voxels/ChunkMesher.h"
#include "Render/Material.h"
#include "Engine.h"
#include <glm/gtc/matrix_transform.hpp>

namespace RR
{
    // PUBLIC ----------------------------------------------------------------------------------------------------------

    ChunkManager::ChunkManager(ChunkGenerator _generator,
        std::shared_ptr<Material> _blockMat,  std::shared_ptr<Material> _vegMat)
        : m_blockMat(std::move(_blockMat)), m_vegMat(std::move(_vegMat)),
          m_generator(std::move(_generator))
    {
    }

    ChunkManager::~ChunkManager()
    = default;

    void ChunkManager::Update(const vec3& _cameraPos)
    {
        // Updates which chunks are drawing from camera pos
        const CHUNK::Coord centre = WorldToChunk(_cameraPos);
        if (m_lastCoords == centre && !m_firstFrame) return;

        EnsureGenerated(centre);
        EnsureMeshed(centre);
        UnloadFar(centre);
        m_lastCoords = centre;
        m_firstFrame = false;
    }

    void ChunkManager::SubmitDraws()
    {
        auto& queue = Engine::GetInstance().GetRenderQueue();

        for (auto& [coord, chunk] : m_chunks)
        {
            if (!chunk->mesh) continue;

            auto chunkMatrix = glm::translate(mat4(1.0f),
                vec3(coord.x * CHUNK::kSizeX, 0.0f, coord.z * CHUNK::kSizeZ));

            // Create and submit render command for that chunk
            RenderCommand command;
            command.material    = m_blockMat.get();
            command.mesh        = chunk->mesh.get();
            command.modelMatrix = chunkMatrix;
            command.color       = vec3(1.0f);

            queue.Submit(command);
        }

        // Render vegetation AFTER opaque blocks
        for (auto& [coord, chunk] : m_chunks)
        {
            if (!chunk->vegMesh) continue;

            auto model = glm::translate(mat4(1.0f),
                vec3(coord.x*CHUNK::kSizeX, 0.0f, coord.z*CHUNK::kSizeZ));

            // Create and submit render command for vegetation on this chunk
            RenderCommand command;
            command.material    = m_vegMat.get();
            command.mesh        = chunk->vegMesh.get();
            command.modelMatrix = model;
            command.color       = vec3(1.0f);

            queue.Submit(command);
        }
    }

    void ChunkManager::Clear()
    {
        // Triggers regeneration from scratch
        m_chunks.clear();
        m_firstFrame = true;
    }

    void ChunkManager::SetFancyLeaves(bool _fancyLeaves)
    {
        m_fancyLeaves = _fancyLeaves;
        Clear();
    }

    bool ChunkManager::GetFancyLeaves() const
    {
        return m_fancyLeaves;
    }

    void ChunkManager::SetRenderDistance(float _distance)
    {
        m_meshRadius = _distance;
    }

    bool ChunkManager::GetRenderDistance() const
    {
        return m_meshRadius;
    }

    // PRIVATE ---------------------------------------------------------------------------------------------------------

    void ChunkManager::UnloadFar(CHUNK::Coord _centre)
    {
        const int range = m_meshRadius + 2;

        // Form an outer rings where chunks that fell out are unloaded
        for (auto it = m_chunks.begin(); it != m_chunks.end();)
        {
            const int dist = std::max(std::abs(it->first.x - _centre.x),
                                      std::abs(it->first.z - _centre.z));

            // erase out of bound chunk
            if (dist > range)
                it = m_chunks.erase(it);
            else
                ++it;
        }
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
        ChunkMeshes meshes = MeshChunk(_chunk, borders, m_fancyLeaves);

        // mesh for chunks
        auto& chunkMesh = meshes.opaque;
        _chunk.mesh = std::make_unique<Mesh>(chunkMesh.layout, chunkMesh.vertices, chunkMesh.indices);

        // mesh for vegetation
        auto& meshVeg = meshes.veg;
        if (!meshVeg.Empty())
        {
            _chunk.vegMesh = std::make_unique<Mesh>(meshVeg.layout, meshVeg.vertices, meshVeg.indices);
        }

        // chunk marked as meshed
        _chunk.state = CHUNK::STATE::MESHED;
    }

    void ChunkManager::EnsureGenerated(CHUNK::Coord _centre)
    {
        const int range = m_meshRadius + 1;
        for (int z = _centre.z - range; z <= _centre.z + range; ++z)
        {
            for (int x = _centre.x - range; x <= _centre.x + range; ++x)
            {
                // Skip if already generated
                if (!GetChunk({x, z}))
                {
                    GenerateChunk({x, z});
                }
            }
        }
    }

    void ChunkManager::EnsureMeshed(CHUNK::Coord _centre)
    {
        const int range = m_meshRadius;
        for (int z = _centre.z - range; z <= _centre.z + range; ++z)
        {
            for (int x = _centre.x - range; x <= _centre.x + range; ++x)
            {
                Chunk* chunk = GetChunk({x, z});

                // Only mesh if chunk is logged generated but not meshed
                if (!chunk || chunk->state != CHUNK::STATE::GENERATED) continue;
                // Skip if neighbors are not generated yet
                if (!NeighboursGenerated({x, z})) continue;

                // Mesh
                BuildChunkMesh(*chunk);
            }
        }
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

    CHUNK::Coord ChunkManager::WorldToChunk(const vec3& _pos)
    {
        // floor-divide to prevent neg cords from 0ing
        return{
            static_cast<int>(std::floor(_pos.x / CHUNK::kSizeX)),
            static_cast<int>(std::floor(_pos.z / CHUNK::kSizeZ))
        };
    }
}
