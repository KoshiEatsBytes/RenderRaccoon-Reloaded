
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
        const int tiled     = EnsureNodes(centre);

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

            // if a real chunk is over this cord it wins, therefore skip
            const auto cit = m_chunks.find(key.origin);
            if (cit != m_chunks.end() && cit->second->mesh) continue;

            const vec3 min (key.origin.x * CHUNK::kSizeX, 0.0f,
                            key.origin.z * CHUNK::kSizeZ);

            const vec3 max (min.x + key.footprint * CHUNK::kSizeX, CHUNK::kSizeY,
                            min.z + key.footprint * CHUNK::kSizeZ);


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
        m_liveKeys.clear();
        m_desiredKeys.clear();
        m_buildQueue.clear();
        m_firstFrame = true;
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
        for (const auto& [key, tile] : m_lodTiles)
        {
            int nearest = NearestDist(_centre, key.origin, key.footprint);

            if (tile.mesh && nearest <= m_meshRadius)
            {
                ++covered;
            }
        }

        return static_cast<float>(covered) / static_cast<float>(total);
    }

    int ChunkManager::ComputeCoreMask(const LodNodeKey &_key, CHUNK::Coord _centre) const
    {
        // nodes are never/cant be core adjacent
        if (_key.footprint > 1) return 0;

        const auto inCore = [&](int _dx, int _dz) -> bool
        {
            return std::max(std::abs(_key.origin.x + _dx - _centre.x),
                            std::abs(_key.origin.z + _dz - _centre.z))
                              <= m_coreRadius;
        };

        // builds 4 neighbor mask
        int mask = 0;
        if (inCore(-1, 0)) mask |= 0x1;
        if (inCore( 1, 0)) mask |= 0x2;
        if (inCore( 0,-1)) mask |= 0x4;
        if (inCore( 0, 1)) mask |= 0x8;
        return mask;
    }

    // check if tile at coord is ready to be rendered
    bool ChunkManager::TileCoveringReady(CHUNK::Coord _coord) const
    {
        for (int lvl = 1; lvl <= m_maxLevel; ++lvl)
        {
            const auto it = m_lodTiles.find({_coord, lvl, 1});

            if (it != m_lodTiles.end() && it->second.mesh) return true;
        }
        return false;
    }

    void ChunkManager::StoreTile(const LodNodeKey& _key, LodTile&& _tile)
    {
        m_lodTiles[_key] = std::move(_tile);
        m_liveKeys.insert(_key);
    }

    // iterator fridnly erase, return next
    auto ChunkManager::EraseTile(TileMap::iterator _it) -> TileMap::iterator
    {
        m_liveKeys.erase(_it->first);
        return m_lodTiles.erase(_it);
    }

    void ChunkManager::BuildTile(const LodNodeKey& _key, int _coreMask)
    {
        const LodMeshResult data = m_lodMesher(_key, _coreMask);

        LodTile tile;
        tile.coreEdges = _coreMask;
        tile.mesh = std::make_unique<Mesh>(data.surface.layout, data.surface.vertices, data.surface.indices);

        // assemble proxies mesh if present
        if (!data.proxies.indices.empty())
        {
            tile.proxyMesh = std::make_unique<Mesh>(data.proxies.layout, data.proxies.vertices, data.proxies.indices);
        }

        StoreTile(_key, std::move(tile));
    }

    // any per-chunk tile at this cord
    bool ChunkManager::AnyTileAt(CHUNK::Coord _coord) const
    {
        for (int level = 1; level <= m_maxLevel; ++level)
        {
            if (m_liveKeys.contains({_coord, level, 1})) return true;
        }
        return false;
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
            int nearest = NearestDist(_centre, it->first.origin, it->first.footprint);

            if (nearest > tileRange)
                it = EraseTile(it);
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

            // +1 to prevent a null ring between 0 and 1
            if (dist > m_coreRadius + 1 && TileCoveringReady(it->first))
                it = m_chunks.erase(it);
            else
                ++it;
        }

        // a tile whos inside the core, drop once chunk is meshed
        for (auto it = m_lodTiles.begin(); it != m_lodTiles.end(); )
        {
            const LodNodeKey& key  = it->first;
            const int         dist = NearestDist(_centre, key.origin, key.footprint);

            const Chunk* chunk = GetChunk(key.origin);
            const bool chunkReady = chunk && chunk->state == CHUNK::STATE::MESHED;

            // drop current chunk if replacement tile is ready
            if (key.footprint == 1 && dist <= m_coreRadius && chunkReady)
                it = EraseTile(it);
            else
                ++it;
        }
    }

    // after a replacement if built and stores, drop any other level present for this orgin
    void ChunkManager::RetireCovered(const LodNodeKey& _key)
    {
        for (int level = 1; level <= m_maxLevel; ++level)
        {
            if (level == _key.level) continue;

            const LodNodeKey old { _key.origin, level, 1 };

            if (m_lodTiles.erase(old))
                m_liveKeys.erase(old);
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

    int ChunkManager::EnsureNodes(CHUNK::Coord _centre)
    {
        using namespace CHUNK;

        if (!m_lodEnabled) return 0;

        int built = 0;

        // core - fill with l1 when not yet ready
        for (const Coord offset : m_genOffsets)
        {
            // Dont stall on single thread
            if (built >= kTileBudget) break;

            const int dist = chessDist(offset);
            if (dist >  m_coreRadius) break;

            const Coord cords {
                _centre.x + offset.x,
                _centre.z + offset.z
            };

            // real chunk meshed, dont fill
            const Chunk* chunk = GetChunk(cords);
            if (chunk && chunk->state == STATE::MESHED) continue;

            // existing tiles cover until mesh is ready
            if (AnyTileAt(cords)) continue;

            BuildTile({cords, 1, 1}, 0);
            ++built;
        }

        // check if any budget left
        if (built >= kTileBudget) return built;

        // select levels and hysteresis, what exists beyond core
        const RingParams params = BuildRingParams();

        m_desiredKeys.clear();
        SelectNodes(_centre, params, &m_liveKeys, m_desiredKeys);

        // desired vs live, stale or missing entry means needs to be built
        m_buildQueue.clear();

        for (const LodNodeKey& key : m_desiredKeys)
        {
            const int coreMask = ComputeCoreMask(key, _centre);

            const auto it = m_lodTiles.find(key);

            // skip up to date
            if (it != m_lodTiles.end() && it->second.coreEdges == coreMask) continue;

            m_buildQueue.push_back({key, coreMask});
        }

        const auto nearer = [&](const PendingBuild& _a, const PendingBuild& _b)
        {
            return NearestDist(_centre, _a.key.origin, _a.key.footprint)
                   < NearestDist(_centre, _b.key.origin, _b.key.footprint);
        };

        // only build first few per frame - partial sort
        const sizeT toBuild = std::min(m_buildQueue.size(),
            static_cast<std::size_t>(kTileBudget - built));

        std::ranges::partial_sort(m_buildQueue,
            m_buildQueue.begin() + toBuild, nearer);

        // build sorted
        for (sizeT i = 0; i < toBuild; i++)
        {
            BuildTile(m_buildQueue[i].key, m_buildQueue[i].coreMask);
            RetireCovered(m_buildQueue[i].key);
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
