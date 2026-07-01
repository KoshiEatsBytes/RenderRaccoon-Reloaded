
#include "ChunkManager.h"

#include <cmath>
#include <algorithm>

#include "Render/Mesh.h"
#include "Render/RenderQueue.h"
#include "Render/Voxels/ChunkMesher.h"
#include "Render/Material.h"
#include "Engine.h"
#include <glm/gtc/matrix_transform.hpp>

#include "miniaudio.h"

namespace RR
{
    // LOCAL -----------------------------------------------------------------------------------------------------------

    // calculates chebyshev distance (chessboard)
    static int chessDist(CHUNK::Coord _chunk)
    {
        return std::max(std::abs(_chunk.x), std::abs(_chunk.z));
    }

    // PUBLIC ----------------------------------------------------------------------------------------------------------

    // Inject generator and mesher as lambdas, thread safe
    ChunkManager::ChunkManager(ChunkGenerator _generator, LodMesher _mesher,
        std::shared_ptr<Material> _blockMat,  std::shared_ptr<Material> _vegMat)
        : m_generator(std::move(_generator)), m_lodMesher(std::move(_mesher)),
          m_blockMat(std::move(_blockMat)), m_vegMat(std::move(_vegMat))
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
        const int tiled     = EnsureTiles(centre);

        // retire old visual once its replacement is uplaoded
        RetireReplaced(centre);

        if (generated == 0 && meshed == 0 && tiled == 0)
        {
            m_streamingIdle = true;
            m_coverage      = 1.0f;
        }
        else
        {
            m_coverage = ComputeCoverage(centre);
        }
    }

    void ChunkManager::SubmitDraws(const Frustum& _frustum)
    {
        auto& queue = Engine::GetInstance().GetRenderQueue();

        const auto IntersectsFrustum = [_frustum](const CHUNK::Coord& _coord) -> bool
        {
            // compute chunk "hitbox"
            const vec3 min (_coord.x * CHUNK::kSizeX, 0.0f, _coord.z * CHUNK::kSizeZ);
            const vec3 max (min.x + CHUNK::kSizeX, CHUNK::kSizeY, min.z + CHUNK::kSizeZ);
            // Check if inside camera frustum with aabb
            return _frustum.IntersectsAABB(min, max);
        };

        for (auto& [coord, chunk] : m_chunks)
        {
            if (!chunk->mesh) continue;

            // if chunk aabbs outside frustum discard render
            if (!IntersectsFrustum(coord)) continue;

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

        // Distant LOD surface tiles
        for (auto& [coord, tile] : m_lodTiles)
        {
            if (!tile.mesh) continue;

            // if a real chunk is over this cord it wins, therefore skip
            const auto cit = m_chunks.find(coord);
            if (cit != m_chunks.end() && cit->second->mesh) continue;

            if (!IntersectsFrustum(coord)) continue;

            auto model = glm::translate(mat4(1.0f),
                vec3(coord.x * CHUNK::kSizeX, 0.0f, coord.z * CHUNK::kSizeZ));

            RenderCommand command;
            command.material    = m_blockMat.get();
            command.mesh        = tile.mesh.get();
            command.modelMatrix = model;
            command.color       = vec3(1.0f);
            queue.Submit(command);

            // if mesh renders, render poxies too
            if (tile.proxyMesh)
            {
                RenderCommand proxyCmd;
                proxyCmd.material    = m_blockMat.get();
                proxyCmd.mesh        = tile.proxyMesh.get();
                proxyCmd.modelMatrix = model;
                proxyCmd.color       = vec3(1.0f);
                queue.Submit(proxyCmd);
            }
        }


        // Render vegetation AFTER opaque blocks
        for (auto& [coord, chunk] : m_chunks)
        {
            if (!chunk->vegMesh) continue;

            // if chunk aabbs outside frustum discard render
            if (!IntersectsFrustum(coord)) continue;

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
        m_lodTiles.clear();
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
        NormalizeRanges();
    }

    int ChunkManager::GetRenderDistance() const
    {
        return m_meshRadius;
    }

    void ChunkManager::SetLodEnabled(bool _enabled)
    {
        m_lodEnabled = _enabled;
        NormalizeRanges();
        Clear();
    }

    void ChunkManager::SetCoreRadius(int _radius)
    {
        m_coreRadius = _radius;
        NormalizeRanges();
        Clear();
    }

    // LOD needs at least one ringn RD must exceed the pure-voxel core, else the tile band
    void ChunkManager::NormalizeRanges()
    {
        if (m_lodEnabled && m_meshRadius <= m_coreRadius)
            m_meshRadius = m_coreRadius + 1;
    }

    void ChunkManager::SetRingGrowth(float _growth)
    {
        m_ringGrowth = _growth;
        Clear();
    }

    void ChunkManager::SetMaxLevel(int _level)
    {
        m_maxLevel = _level;
        Clear();
    }

    bool ChunkManager::IsChunkMeshedAt(const vec3& _pos)
    {
        const Chunk* chunk = GetChunk(WorldToChunk(_pos));
        return chunk && chunk->state == CHUNK::STATE::MESHED;
    }

    bool ChunkManager::IsStreamingIdle() const
    {
        return m_streamingIdle;
    }

    //returns the coverage cached by Update
    float ChunkManager::GetCoverage() const
    {
        return m_coverage;
    }

    // scan the loaded chunks and count those meshed within the mesh radius.
    float ChunkManager::ComputeCoverage(CHUNK::Coord _centre) const
    {
        // mesh range
        const int span  = 2 * m_meshRadius + 1;
        const int total = span* span;

        if (total <= 0) return 1.0f;

        int covered = 0;
        // coverage of core chunks
        for (const auto& [cords, chunk] : m_chunks)
        {
            if (std::max(std::abs(cords.x - _centre.x), std::abs(cords.z - _centre.z)) <= m_meshRadius &&
                chunk->state == CHUNK::STATE::MESHED)
            {
                ++covered;
            }
        }
        // Coverage of lod tiles
        for (const auto& [cords, tile] : m_lodTiles)
        {
            if (std::max(std::abs(cords.x - _centre.x), std::abs(cords.z - _centre.z)) <= m_meshRadius &&
                tile.mesh)
            {
                ++covered;
            }
        }

        return static_cast<float>(covered) / static_cast<float>(total);
    }

    int ChunkManager::LevelForDistance(int _dist) const
    {
        // full detail in the core, or whenever lod is off
        if (!m_lodEnabled || _dist <= m_coreRadius) return 0;

        int   level = 1;
        float edge  = m_coreRadius * m_ringGrowth;   // outer edge of ring 1

        // step outward by the growth factor until the ring contains _dist
        while (_dist > static_cast<int>(edge) && level < m_maxLevel)
        {
            edge *= m_ringGrowth;
            ++level;
        }

        return level;
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
        const int tileRange = m_meshRadius + 2;

        // only remove out of view, not inbetween lod bands
        for (auto it = m_chunks.begin(); it != m_chunks.end();)
        {
            const int dist = std::max(std::abs(it->first.x - _centre.x),
                                      std::abs(it->first.z - _centre.z));

            if (dist > tileRange)
                it = m_chunks.erase(it);
            else
                ++it;
        }

        // remove far lod bands
        for (auto it = m_lodTiles.begin(); it != m_lodTiles.end();)
        {
            const int dist = std::max(std::abs(it->first.x - _centre.x),
                                      std::abs(it->first.z - _centre.z));

            if (dist > tileRange)
                it = m_lodTiles.erase(it);
            else
                ++it;
        }
    }

    void ChunkManager::RetireReplaced(CHUNK::Coord _centre)
    {
        // a chunk that translated from lod0 to 1 waits for the tile to be ready to be replaced
        for (auto it = m_chunks.begin(); it != m_chunks.end();)
        {
            const int dist = std::max(std::abs(it->first.x - _centre.x),
                                      std::abs(it->first.z - _centre.z));

            const auto tileIt = m_lodTiles.find(it->first);

            // log if tile is ready
            const bool tileReady = tileIt != m_lodTiles.end() && tileIt->second.mesh;

            // +1 to prevent a null ring between 0 and 1
            if (dist > m_coreRadius + 1 && tileReady)
                it = m_chunks.erase(it);
            else
                ++it;
        }

        // a tile whos inside the core, drop once chunk is meshed
        for (auto it = m_lodTiles.begin(); it != m_lodTiles.end(); )
        {
            const int  dist  = std::max(std::abs(it->first.x - _centre.x),
                                        std::abs(it->first.z - _centre.z));

            const Chunk* chunk = GetChunk(it->first);
            const bool chunkReady = chunk && chunk->state == CHUNK::STATE::MESHED;

            // drop current chunk if replacement tile is ready
            if (dist <= m_coreRadius && chunkReady)
                it = m_lodTiles.erase(it);
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

    int ChunkManager::EnsureTiles(CHUNK::Coord _centre)
    {
        using namespace CHUNK;

        if (!m_lodEnabled) return 0;

        int built = 0;
        for (const Coord offset : m_genOffsets)
        {
            // Dont stall on single thread
            if (built >= kTileBudget) break;

            const int dist = chessDist(offset);
            if (dist >  m_meshRadius) break;

            const Coord cords {
                _centre.x + offset.x,
                _centre.z + offset.z
            };

            const auto it = m_lodTiles.find(cords);
            const int current = it != m_lodTiles.end() ? it->second.level : -1;

            int lodLevel;
            if (dist <= m_coreRadius)
            {
                // core zone, cheap until main has been rendered
                const Chunk* chunk = GetChunk(cords);
                // real chunk is ready
                if (chunk && chunk->state == STATE::MESHED) continue;
                if (current >= 0) continue;

                // until then use cheap chunks
                lodLevel = 1;
            }
            else
            {

                // only change lod level when the distance has cleared the specified threshold
                lodLevel = LevelForDistance(dist);
                if (current >= 0)
                {
                    if (lodLevel > current)
                    {
                        lodLevel = LevelForDistance(dist - m_lodHysteresis);
                    }
                    else if (lodLevel < current)
                    {
                        lodLevel = LevelForDistance(dist + m_lodHysteresis);
                    }
                }
            }

            // skip if already correct
            if (current == lodLevel) continue;

            // already tiled
            if (it != m_lodTiles.end() && it->second.level == lodLevel)
                continue;

            // extract mesh surface
            const LodMeshResult data = m_lodMesher(cords, lodLevel);
            LodTile tile;

            // assemble surface mesh
            tile.cords = cords;
            tile.level = lodLevel;
            tile.mesh  = std::make_unique<Mesh>(data.surface.layout, data.surface.vertices, data.surface.indices);

            // assemble proxies mesh (if present)
            if (!data.proxies.indices.empty())
            {
                tile.proxyMesh = std::make_unique<Mesh>(data.proxies.layout, data.proxies.vertices, data.proxies.indices);
            }

            m_lodTiles[cords] = std::move(tile);
            ++built;
        }

        return built;
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
