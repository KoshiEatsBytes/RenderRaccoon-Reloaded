
#pragma once
#include <vector>
#include <memory>
#include <unordered_map>

#include "Helpers/Types.h"
#include "Render/Frustum.h"
#include "Voxels/Chunk.h"
#include "Voxels/ChunkData.h"
#include "Voxels/LodTile.h"
#include "Render/Voxels/ChunkMesher.h"
#include "Render/Voxels/SurfaceMesher.h"
#include "Voxels/LodNodeSelect.hpp"

namespace RR
{
    // lod mesh data for render
    struct LodMeshResult
    {
        MeshData surface;
        MeshData proxies;
    };

    struct PendingBuild
    {
        LodNodeKey key;
        int  coreMask;
        bool covered;
    };

    using LodMesher = std::function<LodMeshResult(LodNodeKey, int)>;
    using TileMap   = std::unordered_map<LodNodeKey, LodTile, LodNodeKeyHash>;
    using ChunkMap  = std::unordered_map<CHUNK::Coord, std::unique_ptr<Chunk>, CHUNK::CoordHash>;

    class Material;
    class ChunkManager
    {
    public:
        ChunkManager(ChunkGenerator _generator, LodMesher _mesher,
            std::shared_ptr<Material> _blockMat,
            std::shared_ptr<Material> _vegMat);
        ~ChunkManager();

        void Update(const vec3& _cameraPos);
        void SubmitDraws(const Frustum& _frustum);
        void Clear();

        void SetRenderDistance(int _distance);
        int  GetRenderDistance() const;

        void SetFancyLeaves(bool _fancyLeaves);

        // Lod tuning
        void SetLodEnabled(bool _enabled);
        void SetAggregationEnabled(bool _enabled);
        void SetCoreRadius(int _radius);
        void SetRingGrowth(float _growth);
        void SetMaxLevel(int _level);

        // Pure-voxel core radius, lod needs higher
        static constexpr int kDefaultCoreRadius = 16;

        bool IsChunkMeshedAt(const vec3& _pos);
        bool IsStreamingIdle() const;
        float GetCoverage() const;

    private:
        RingParams BuildRingParams() const;
        void RebuildRingOffset();
        void NormalizeRanges();
        void CommitFlips();

        void UnloadFar(CHUNK::Coord _centre);
        void GenerateChunk(CHUNK::Coord _coord);
        void BuildChunkMesh(Chunk& _chunk);

        int   EnsureGenerated(CHUNK::Coord _centre);
        int   EnsureMeshed(CHUNK::Coord _centre);
        int   EnsureNodes(CHUNK::Coord _centre);
        bool  NeighboursGenerated(CHUNK::Coord _coord);
        float ComputeCoverage(CHUNK::Coord _centre) const;

        void RetireReplaced(CHUNK::Coord _centre);
        void EraseCoveredBy(const LodNodeKey& _key);

        bool TileCoveringReady(CHUNK::Coord _coord) const;

        void StoreTile(const LodNodeKey& _key, LodTile&& _tile);
        void StorePending(const LodNodeKey& _key, LodTile&& _tile);
        auto EraseTile(TileMap::iterator _it) -> TileMap::iterator;
        auto ErasePending(TileMap::iterator _it) -> TileMap::iterator;
        void ActivatePending(const LodNodeKey& _key);

        LodTile MakeTile(const LodNodeKey& _key, int _coreMask);
        void    BuildTile(const LodNodeKey& _key, int _coreMask);
        int     ComputeCoreMask(const LodNodeKey& _key, CHUNK::Coord _centre) const;

        bool ChunkReadyAt(CHUNK::Coord _coord) const;
        bool AnyTileAt(CHUNK::Coord _coord) const;
        bool ReadyTileAt(CHUNK::Coord _coord) const;
        bool ReadyNodeAt(CHUNK::Coord _origin, int _level, int _footprint) const;
        bool HasLiveAncestor(const LodNodeKey& _key) const;
        bool CoveredByReplacement(CHUNK::Coord _origin, int _level) const;
        bool AreaCovered(const LodNodeKey& _key) const;

        Chunk* GetChunk(CHUNK::Coord _coord);
        ChunkBorders GatherBorders(CHUNK::Coord _coord);
        static CHUNK::Coord WorldToChunk(const vec3& _pos);

        // Algorithms
        ChunkGenerator m_generator;
        LodMesher      m_lodMesher;

        std::vector<LodNodeKey>   m_desiredKeys;
        std::vector<PendingBuild> m_buildQueue;

        // terrain/visual
        ChunkMap m_chunks;
        TileMap  m_lodTiles;

        // lod ring lifecycle
        TileMap m_pendingTiles;
        NodeSet m_desiredSet;
        NodeSet m_liveKeys;
        bool    m_desiredFresh = false;

        // mats
        std::shared_ptr<Material> m_blockMat;
        std::shared_ptr<Material> m_vegMat;

        std::vector<CHUNK::Coord> m_genOffsets;
        int m_offsetsBuiltForRadius = -1;

        CHUNK::Coord m_lastCoords;
        bool  m_streamingIdle = false;
        bool  m_firstFrame    = true;
        float m_coverage      = 1.0f;

        // Quality settings
        bool m_fancyLeaves        = true;
        bool m_lodEnabled         = false;
        bool m_aggregationEnabled = false;

        // RD settings
        int m_meshRadius = 16; // total RD
        int m_coreRadius = kDefaultCoreRadius; // full detail RD

        // Lod ring shaping
        float m_ringGrowth    = 2.f;
        int   m_maxLevel      = 4;
        int   m_lodHysteresis = 2; // chunks of deadband around each ring

        // Aggregation
        int m_nodingStart   = 2;
        int m_maxLevelClamp = 0;

        // Per frame budget
        static constexpr int kGenBudget  = 1;
        static constexpr int kMeshBudget = 1;

        // lod rings and budgets
        static constexpr int kTileBudget  = 4;
    };
}

