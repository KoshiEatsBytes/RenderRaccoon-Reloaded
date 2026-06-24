
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
    // LOCAL -----------------------------------------------------------------------------------------------------------

    // calculates chebyshev distance (chessboard)
    static int chessDist(CHUNK::Coord _chunk)
    {
        return std::max(std::abs(_chunk.x), std::abs(_chunk.z));
    }

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
        const bool radiusChanged = m_offsetsBuiltForRadius != m_meshRadius;

        // Regen if reloaded or changed RD
        if (radiusChanged) RebuildRingOffset();

        // Log if player has moved
        const CHUNK::Coord centre = WorldToChunk(_cameraPos);
        const bool moved = (centre != m_lastCoords) || m_firstFrame;

        // fire if camera crossed a boundary
        if (moved || radiusChanged)
        {
            UnloadFar(centre);
            m_lastCoords    = centre;
            m_streamingIdle = false;
            m_firstFrame    = false;
        }

        // Nothing to do if camera station and the ring is fully generated
        if (m_streamingIdle) return;

        // check budgets, if 0 CM idle
        const int generated = EnsureGenerated(centre);
        const int meshed    = EnsureMeshed(centre);

        if (generated == 0 && meshed == 0) m_streamingIdle = true;
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

    void ChunkManager::SetRenderDistance(int _distance)
    {
        m_meshRadius = _distance;
    }

    int ChunkManager::GetRenderDistance() const
    {
        return m_meshRadius;
    }

    void ChunkManager::RebuildRingOffset()
    {
        m_genOffsets.clear();

        // chunks to manage around the player + 1
        const int range = m_meshRadius + 1;
        m_genOffsets.reserve((2 * range + 1) * (2 * range + 1));

        // map ring around player
        for (int distZ = -range; distZ <= range; ++distZ)
        {
            for (int distX = -range; distX <= range; ++distX)
            {
                m_genOffsets.push_back({distX, distZ});
            }
        }

        // Closest first, sort nearby chunks first
        std::ranges::sort(m_genOffsets,
            [](CHUNK::Coord _cA, CHUNK::Coord _cB)
            {
                return chessDist(_cA) < chessDist(_cB);
            });

        m_offsetsBuiltForRadius = m_meshRadius;
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



    int ChunkManager::EnsureGenerated(CHUNK::Coord _centre)
    {
        int generated = 0;
        for (const CHUNK::Coord offset : m_genOffsets)
        {
            // discard if budget cap is overrun
            if (generated >= kGenBudget) break;

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
        int meshed = 0;
        for (const CHUNK::Coord offset : m_genOffsets)
        {
            // discard if budget is overrun
            if (meshed >= kMeshBudget) break;

            // check if the chunk is in range
            if (chessDist(offset) > m_meshRadius) break;

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

    // Check neighboring chunks, if all generated return true
    bool ChunkManager::NeighboursGenerated(CHUNK::Coord _coord)
    {
        return GetChunk({_coord.x + 1, _coord.z}) &&
               GetChunk({_coord.x - 1, _coord.z}) &&
               GetChunk({_coord.x, _coord.z + 1}) &&
               GetChunk({_coord.x, _coord.z - 1});
    }

    bool ChunkManager::IsStreamingIdle() const
    {
        return m_streamingIdle;
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
