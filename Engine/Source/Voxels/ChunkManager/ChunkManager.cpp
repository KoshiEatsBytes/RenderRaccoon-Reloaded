
#include "ChunkManager.h"

#include <cmath>
#include <algorithm>
#include <cassert>

#include "Render/Mesh.h"
#include "Render/RenderQueue.h"
#include "Render/Voxels/ChunkMesher.h"
#include "Render/Material.h"
#include "Engine.h"
#include <glm/gtc/matrix_transform.hpp>

namespace RR
{
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
        const int tiled     = EnsureNodes(centre);

        // retire old visual once its replacement is uplaoded
        CommitFlips();
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

        for (auto& [coord, chunk] : m_chunks)
        {
            if (!chunk->mesh) continue;

            const vec3 min (coord.x * CHUNK::kSizeX, 0.0f, coord.z * CHUNK::kSizeZ);
            const vec3 max (min.x + CHUNK::kSizeX, CHUNK::kSizeY, min.z + CHUNK::kSizeZ);

            // if chunk aabbs outside frustum discard render
            if (!_frustum.IntersectsAABB(min, max)) continue;

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
        for (auto& [key, tile] : m_lodTiles)
        {
            if (!tile.mesh) continue;

            int nearest = NearestDist(m_lastCoords, key.origin, key.footprint);

            // chunk wins dedup
            if (key.footprint == 1)
            {
                const auto cit = m_chunks.find(key.origin);
                if (cit != m_chunks.end() && cit->second->mesh) continue;
            }
            // all or nothing for lod nodes
            else if (nearest <= m_coreRadius + 1)
            {
                // only l1 ajacent nodes can overlap real chunks
                bool fullyMeshed = true;
                for (int dz = 0; dz < key.footprint && fullyMeshed; ++dz)
                {
                    for (int dx = 0; dx < key.footprint && fullyMeshed; ++dx)
                    {
                        fullyMeshed = ChunkReadyAt({key.origin.x + dx, key.origin.z + dz});
                    }
                }
                if (fullyMeshed) continue;
            }

            // calc frustum boundaries
            const vec3 min (key.origin.x * CHUNK::kSizeX, 0.0f,
                            key.origin.z * CHUNK::kSizeZ);

            const vec3 max (min.x + key.footprint * CHUNK::kSizeX, CHUNK::kSizeY,
                            min.z + key.footprint * CHUNK::kSizeZ);

            // lifecycle backstop, when l1+ chunks are present at core
            if (!m_streamingIdle && HasLiveAncestor(key))
            {
                if (key.footprint > 1)
                {
                    Warn("[ChunkManager] dedup backstop: node under node at ",
                         key.origin.x, ",", key.origin.z, " L", key.level);
                }
                continue;
            }


            if (!_frustum.IntersectsAABB(min, max)) continue;

            auto model = glm::translate(mat4(1.0f), vec3(min.x, 0.0f, min.z));

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
            const vec3 min (coord.x * CHUNK::kSizeX, 0.0f, coord.z * CHUNK::kSizeZ);
            const vec3 max (min.x + CHUNK::kSizeX, CHUNK::kSizeY, min.z + CHUNK::kSizeZ);

            // if chunk aabbs outside frustum discard render
            if (!_frustum.IntersectsAABB(min, max)) continue;

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
        m_pendingTiles.clear();
        m_liveKeys.clear();
        m_desiredKeys.clear();
        m_desiredSet.clear();
        m_buildQueue.clear();
        m_desiredFresh = false;
        m_firstFrame   = true;
    }

    void ChunkManager::SetFancyLeaves(bool _fancyLeaves)
    {
        m_fancyLeaves = _fancyLeaves;
        Clear();
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

    void ChunkManager::SetAggregationEnabled(bool _enabled)
    {
        m_aggregationEnabled = _enabled;
        NormalizeRanges();
        Clear();
    }

    void ChunkManager::SetCoreRadius(int _radius)
    {
        m_coreRadius = _radius;
        NormalizeRanges();
        Clear();
    }

    void ChunkManager::SetRingGrowth(float _growth)
    {
        m_ringGrowth = _growth;
        NormalizeRanges();
        Clear();
    }

    // 0 is automatic derived from rd
    void ChunkManager::SetMaxLevel(int _level)
    {
        m_maxLevelClamp = _level;
        NormalizeRanges();
        Clear();
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

    // PRIVATE ---------------------------------------------------------------------------------------------------------

    // LOD needs at least one ringn RD must exceed the pure-voxel core, else the tile band
    void ChunkManager::NormalizeRanges()
    {
        if (m_lodEnabled && m_meshRadius <= m_coreRadius)
        {
            m_meshRadius = m_coreRadius + 1;
        }

        int   level = 1;
        float edge  = m_coreRadius * m_ringGrowth;

        // walk edge up to rd
        while (static_cast<int>(edge) < m_meshRadius && level < 8)
        {
            edge *= m_ringGrowth;
            ++level;
        }

        if (m_maxLevelClamp > 0)
        {
            level = std::min(level, m_maxLevelClamp);
        }
        if (!m_aggregationEnabled)
        {
            // per chunk subdivision cap
            level = std::min(level, 4);
        }

        m_maxLevel = std::max(level, 1);
    }

    RingParams ChunkManager::BuildRingParams() const
    {
        assert(m_maxLevel >= 1);
        assert(m_nodingStart >= 2);

        return {
            m_coreRadius,
            m_ringGrowth,
            m_maxLevel,
            m_aggregationEnabled ? m_nodingStart : m_maxLevel + 1, // off, disabled aggregation
            m_meshRadius,
            m_lodHysteresis
        };
    }

    // scan the loaded chunks and count those meshed within the mesh radius.
    float ChunkManager::ComputeCoverage(CHUNK::Coord _centre) const
    {
        // mesh range
        const int span  = 2 * m_meshRadius + 1;
        const int total = span * span;

        if (total <= 0) return 1.0f;

        int covered = 0;
        // coverage of core chunks
        for (const auto& [cords, chunk] : m_chunks)
        {
            int max = std::max(std::abs(cords.x - _centre.x), std::abs(cords.z - _centre.z));

            if (max <= m_meshRadius && chunk->state == CHUNK::STATE::MESHED)
            {
                ++covered;
            }
        }
        // Coverage of lod tiles, nodes cover area of footprint ^ 2 cells
        for (const auto& [key, tile] : m_lodTiles)
        {
            if (!tile.mesh) continue;

            const int xMin = std::max(key.origin.x,                 _centre.x - m_meshRadius);
            const int xMax = std::min(key.origin.x + key.footprint, _centre.x + m_meshRadius + 1);
            const int zMin = std::max(key.origin.z,                 _centre.z - m_meshRadius);
            const int zMax = std::min(key.origin.z + key.footprint, _centre.z + m_meshRadius + 1);

            if (xMax > xMin && zMax > zMin)
            {
                covered += (xMax - xMin) * (zMax - zMin);
            }
        }

        return std::min(1.0f, static_cast<float>(covered) / static_cast<float>(total));
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
            int nearest = NearestDist(_centre, it->first.origin, it->first.footprint);

            if (nearest > tileRange)
                it = EraseTile(it);
            else
                ++it;
        }

        // remove far pending builds
        for (auto it = m_pendingTiles.begin(); it != m_pendingTiles.end();)
        {
            const int nearest = NearestDist(_centre, it->first.origin, it->first.footprint);

            if (nearest > tileRange)
                it = ErasePending(it);
            else
                ++it;
        }
    }

    CHUNK::Coord ChunkManager::WorldToChunk(const vec3& _pos)
    {
        // floor-divide to prevent neg cords from 0ing
        return{
            static_cast<int>(std::floor(_pos.x / CHUNK::kSizeX)),
            static_cast<int>(std::floor(_pos.z / CHUNK::kSizeZ))
        };
    }

    // calculates chebyshev distance (chessboard)
    int ChunkManager::chessDist(CHUNK::Coord _chunk)
    {
        return std::max(std::abs(_chunk.x), std::abs(_chunk.z));
    }
}
