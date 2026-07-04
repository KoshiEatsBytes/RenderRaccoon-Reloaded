
#include "ChunkManager.h"
#include "Helpers/Types.h"
#include "Voxels/Chunk.h"
#include "Voxels/ChunkData.h"
#include "Voxels/LodTile.h"
#include "Render/Voxels/SurfaceMesher.h"
#include "Voxels/LodNodeSelect.hpp"
#include "Render/Mesh.h"

namespace RR
{
    int ChunkManager::EnsureNodes(CHUNK::Coord _centre)
    {
        using namespace CHUNK;

        if (!m_lodEnabled) return 0;

        int budget = kTileBudget;
        if (m_asyncEnabled)
        {
            // fill pool to outstanding gap
            budget = static_cast<int>(m_pool->GetThreadCount()) * kInFlightWorkerMultiplier -
                     static_cast<int>(m_tileInFlight.size());
        }

        int built = 0;

        // core - fill with l1 when not yet ready
        for (const Coord offset : m_genOffsets)
        {
            // Dont stall on single thread
            if (built >= budget) break;

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

            const LodNodeKey key {cords, 1, 1};

            if (m_asyncEnabled)
            {
                // AnyTileAt cant see wip tiles, so guard aganist it
                if (m_tileInFlight.contains(key)) continue;
                SubmitTileJob(key, 0, true);
            }
            else
            {
                BuildTile({cords, 1, 1}, 0);
            }

            ++built;
        }

        // check if any budget left
        if (built >= budget)
        {
            if (m_asyncEnabled)
            {
                return static_cast<int>(m_tileInFlight.size());
            }

            return built;
        }

        // select levels and hysteresis, what exists beyond core
        const RingParams params = BuildRingParams();

        m_desiredKeys.clear();
        SelectNodes(_centre, params, &m_liveKeys, m_desiredKeys);

        // clear previous
        m_desiredSet.clear();
        m_desiredSet.insert(m_desiredKeys.begin(), m_desiredKeys.end());
        m_desiredFresh = true;

        // desired vs live, stale or missing entry means needs to be built
        m_buildQueue.clear();
        for (const LodNodeKey& key : m_desiredKeys)
        {
            const int coreMask = ComputeCoreMask(key, _centre);

            // wip also counts as "already built"
            if (m_asyncEnabled && m_tileInFlight.contains(key)) continue;

            // pending and active count as "already built"
            const auto activeIt  = m_lodTiles.find(key);
            if (activeIt != m_lodTiles.end() && activeIt->second.coreEdges == coreMask)
                continue;

            const auto pendIt = m_pendingTiles.find(key);
            if (pendIt != m_pendingTiles.end() && pendIt->second.coreEdges == coreMask)
                continue;

            m_buildQueue.push_back({key, coreMask, AreaCovered(key)});
        }

        // weighted perceptual priority, generate close first
        const auto priority = [&](const PendingBuild& _p)
        {
            const int dist = NearestDist(_centre, _p.key.origin, _p.key.footprint);
            return dist + (_p.covered ? kCoveredPenalty : 0);
        };

        const auto nearer = [&](const PendingBuild& _a, const PendingBuild& _b)
        {
            const int prioA = priority(_a);
            const int prioB = priority(_b);

            if (prioA != prioB) return prioA < prioB;

            if (_a.key.origin.x != _b.key.origin.x)
                return _a.key.origin.x < _b.key.origin.x;

            return _a.key.origin.z < _b.key.origin.z;
        };

        // only build first few per frame - partial sort
        const sizeT toBuild = std::min(m_buildQueue.size(),
            static_cast<std::size_t>(budget - built));

        std::ranges::partial_sort(m_buildQueue,
            m_buildQueue.begin() + toBuild, nearer);

        // build sorted
        for (sizeT i = 0; i < toBuild; i++)
        {
            const LodNodeKey& key  = m_buildQueue[i].key;
            const int         mask = m_buildQueue[i].coreMask;

            if (m_asyncEnabled)
            {
                SubmitTileJob(key, mask, false);
            }
            else
            {
                LodTile tile = MakeTile(key, mask);

                // in place mask rebuild, no gaps in the process
                if (m_lodTiles.contains(key))
                    StoreTile(key, std::move(tile));
                else
                    StorePending(key, std::move(tile));
            }

            ++built;
        }

        return m_asyncEnabled ? static_cast<int>(m_tileInFlight.size()) : built;
    }

    void ChunkManager::CommitFlips()
    {
        // discard on fresh
        if (!m_desiredFresh) return;
        m_desiredFresh = false;

        // camera re-backed inside request tile, no longer needed, discard
        // request and dont draw
        for (auto it = m_pendingTiles.begin(); it != m_pendingTiles.end();)
        {
            if (!m_desiredSet.contains(it->first))
                it = ErasePending(it);
            else
                ++it;
        }

        // activate pending key no blocked by an ancestor
        std::vector<LodNodeKey> ready;
        for (const auto& [key, tile] : m_pendingTiles)
        {
            if (!HasLiveAncestor(key))
            {
                ready.push_back(key);
            }
        }

        // activate pending keys
        for (const LodNodeKey& key : ready)
        {
            ActivatePending(key);
            EraseCoveredBy(key);
        }

        // Refine flips, a noce whos no longer desired retires only once
        // its footprint is covered by finer chunks (or core)
        std::vector<LodNodeKey> stale;

        for (const auto& [key, tile] : m_lodTiles)
        {
            if (key.footprint > 1 && !m_desiredSet.contains(key))
                stale.push_back(key);
        }

        for (const LodNodeKey& key : stale)
        {
            if (!CoveredByReplacement(key.origin, key.level)) continue;

            // collect, activations mutates the mapp
            std::vector<LodNodeKey> children;
            for (const auto& [pKey, pTile] : m_pendingTiles)
            {
                // bruh
                const bool inside =
                    pKey.level < key.level                       &&
                    pKey.origin.x >= key.origin.x                &&
                    pKey.origin.x < key.origin.x + key.footprint &&
                    pKey.origin.z >= key.origin.z                &&
                    pKey.origin.z < key.origin.z + key.footprint;

                if (inside) children.push_back(pKey);
            }

            for (const LodNodeKey& child : children)
                ActivatePending(child);

            if (auto it = m_lodTiles.find(key); it != m_lodTiles.end())
                EraseTile(it);
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
    void ChunkManager::EraseCoveredBy(const LodNodeKey& _key)
    {
        const auto eraseBoth = [&](const LodNodeKey& _old)
        {
            if (_old == _key) return;

            auto actIt = m_lodTiles.find(_old);
            if (actIt != m_lodTiles.end())
            {
                EraseTile(actIt);
            }

            auto pendIt = m_pendingTiles.find(_old);
            if (pendIt != m_pendingTiles.end())
            {
                ErasePending(pendIt);
            }
        };

        if (_key.footprint == 1)
        {
            // 1 to 1 replacement, same origin, every other levle
            for (int level = 1; level <= m_maxLevel; ++level)
            {
                if (level != _key.level)
                {
                    eraseBoth({_key.origin, level, 1});
                }
            }

            return;
        }

        // node - iterate every per chunk key inside footprint, erase
        for (int dz = 0; dz < _key.footprint; ++dz)
        {
            for (int dx = 0; dx < _key.footprint; ++dx)
            {
                for (int level = 1; level <= m_maxLevel; ++level)
                {
                    eraseBoth({{
                        _key.origin.x + dx,
                        _key.origin.z + dz
                        },
                        level, 1});
                }
            }
        }

        // Same for every finer node whos grid fall inside footprint
        for (int level = m_nodingStart; level < _key.level; ++level)
        {
            const int footprint = 1 << (level - m_nodingStart + 1);

            for (int dz = 0; dz < _key.footprint; dz += footprint)
            {
                for (int dx = 0; dx < _key.footprint; dx += footprint)
                {
                    eraseBoth({{
                            _key.origin.x + dx,
                            _key.origin.z + dz
                        }, level, footprint});
                }
            }
        }
    }

    LodTile ChunkManager::MakeTile(const LodNodeKey& _key, int _coreMask)
    {
        return UploadTile(_coreMask, m_lodMesher(_key, _coreMask));
    }

    LodTile ChunkManager::UploadTile(int _coreMask, LodMeshResult&& _data)
    {
        // Mesh tile
        LodTile tile;
        tile.coreEdges = _coreMask;
        tile.mesh = std::make_unique<Mesh>(_data.surface.layout, _data.surface.vertices, _data.surface.indices);

        // if present, mesh proxies
        if (!_data.proxies.indices.empty())
        {
            tile.proxyMesh = std::make_unique<Mesh>(_data.proxies.layout, _data.proxies.vertices, _data.proxies.indices);
        }

        return tile;
    }

     void ChunkManager::BuildTile(const LodNodeKey& _key, int _coreMask)
    {
        StoreTile(_key, std::move(MakeTile(_key, _coreMask)));
    }

    void ChunkManager::StoreTile(const LodNodeKey& _key, LodTile&& _tile)
    {
        m_lodTiles[_key] = std::move(_tile);
        m_liveKeys.insert(_key);
    }

    void ChunkManager::StorePending(const LodNodeKey &_key, LodTile &&_tile)
    {
        m_pendingTiles[_key] = std::move(_tile);
        m_liveKeys.insert(_key);
    }

    // iterator fridnly erase, return next
    auto ChunkManager::EraseTile(TileMap::iterator _it) -> TileMap::iterator
    {
        m_liveKeys.erase(_it->first);
        return m_lodTiles.erase(_it);
    }

    auto ChunkManager::ErasePending(TileMap::iterator _it) -> TileMap::iterator
    {
        m_liveKeys.erase(_it->first);
        return m_pendingTiles.erase(_it);
    }

    void ChunkManager::ActivatePending(const LodNodeKey& _key)
    {
        auto it = m_pendingTiles.find(_key);

        if (it == m_pendingTiles.end()) return;

        // live unchanghed
        m_lodTiles[_key] = std::move(it->second);
        m_pendingTiles.erase(it);
    }

    void ChunkManager::SubmitTileJob(const LodNodeKey &_key, int _coreMask, bool _coreFill)
    {
        m_tileInFlight.insert(_key);

        m_pool->Submit([this,
            key   = _key,
            mask  = _coreMask,
            fill  = _coreFill,
            epoch = m_epoch]
        {
            // extract surface, mesh, and proxies all at once, off thread
            LodMeshResult data = m_lodMesher(key, mask);

            std::lock_guard lock(m_resultMutex);
            m_tileResults.push_back({
                key,
                std::move(data),
                mask,
                fill,
                epoch
            });
        });
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

    // any per-chunk tile at this cord
    bool ChunkManager::AnyTileAt(CHUNK::Coord _coord) const
    {
        for (int level = 1; level <= m_maxLevel; ++level)
        {
            if (m_liveKeys.contains({_coord, level, 1})) return true;
        }
        return false;
    }

    // per chunk tile at this cord, any level or pend
    bool ChunkManager::ReadyTileAt(CHUNK::Coord _coord) const
    {
        for (int level = 1; level <= m_maxLevel; ++level)
        {
            const LodNodeKey key {_coord, level, 1};

            if (m_lodTiles.contains(key) || m_pendingTiles.contains(key))
                return true;
        }
        return false;
    }

    // node present in either tiles or pending
    bool ChunkManager::ReadyNodeAt(CHUNK::Coord _origin, int _level, int _footprint) const
    {
        const LodNodeKey key {
            _origin, _level, _footprint
        };

        return m_lodTiles.contains(key) || m_pendingTiles.contains(key);
    }

    // bigger node covering this key origin
    bool ChunkManager::HasLiveAncestor(const LodNodeKey &_key) const
    {
        for (int level = std::max(_key.level + 1, m_nodingStart); level <= m_maxLevel; ++level)
        {
            const int footpritn = 1 << (level - m_nodingStart + 1);

            const CHUNK::Coord snapped {
                SnapDown(_key.origin.x, footpritn),
                SnapDown(_key.origin.z, footpritn)
            };

            if (m_lodTiles.contains({snapped, level, footpritn}))
                return true;
        }
        return false;
    }

    // check if origin + footprint is reaby by finer chunsk
    bool ChunkManager::CoveredByReplacement(CHUNK::Coord _origin, int _level) const
    {
        if (_level < m_nodingStart)
        {
            return ChunkReadyAt(_origin) || ReadyTileAt(_origin);
        }

        int childFootprint = 1;

        if (_level - 1 >= m_nodingStart)
        {
            childFootprint = 1 << (_level - 1 - m_nodingStart + 1);
        }

        for (int dz = 0; dz <= 1; ++dz)
        {
            for (int dx = 0; dx <= 1; ++dx)
            {
                const CHUNK::Coord oc {
                    _origin.x + dx * childFootprint,
                    _origin.z + dz * childFootprint
                };

                // gnode keys only exist at node levels
                if (_level - 1 >= m_nodingStart &&
                    ReadyNodeAt(oc, _level - 1, childFootprint))
                    continue;

                // otherwise cover deeper
                if (CoveredByReplacement(oc, _level - 1))
                    continue;

                return false;
            }
        }
        return true;
    }

    bool ChunkManager::AreaCovered(const LodNodeKey &_key) const
    {
        // Parent still drawn
        if (HasLiveAncestor(_key)) return true;

        // probe floor directly
        if (_key.footprint == 1)
        {
            return ChunkReadyAt(_key.origin) || ReadyTileAt(_key.origin);
        }

        // finer set to draw
        return CoveredByReplacement(_key.origin, _key.level);
    }
}