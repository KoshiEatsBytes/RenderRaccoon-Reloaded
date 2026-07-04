
#pragma once
#include <vector>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <unordered_set>

#include "Helpers/Types.h"
#include "Render/Frustum.h"
#include "Voxels/Chunk.h"
#include "Voxels/ChunkData.h"
#include "Voxels/LodTile.h"
#include "Render/Voxels/ChunkMesher.h"
#include "Render/Voxels/SurfaceMesher.h"
#include "Threading/WorkerPool.h"
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

    // generation result for workers
    struct GenResult
    {
        std::shared_ptr<Chunk> chunk;
        uInt64 epoch;
    };

    // meshing result for workers
    struct MeshResult
    {
        std::shared_ptr<const Chunk> chunk;
        ChunkMeshes meshes;
        uInt64      epoch;
    };

    using LodMesher = std::function<LodMeshResult(LodNodeKey, int)>;
    using TileMap   = std::unordered_map<LodNodeKey, LodTile, LodNodeKeyHash>;
    using ChunkMap  = std::unordered_map<CHUNK::Coord, std::shared_ptr<Chunk>, CHUNK::CoordHash>;

    class Material;

    // Main chunk manager, implementation split across 3 translation units
    class ChunkManager
    {
    public:
        ChunkManager(ChunkGenerator _generator, LodMesher _mesher,
            std::shared_ptr<Material> _blockMat,
            std::shared_ptr<Material> _vegMat);
        ~ChunkManager();

        // Frame loop
        void Update(const vec3& _cameraPos);
        void SubmitDraws(const Frustum& _frustum);
        void Clear();

        void SetRenderDistance(int _distance);
        int  GetRenderDistance() const;
        void SetFancyLeaves(bool _fancyLeaves);

        // toggles
        void SetLodEnabled(bool _enabled);
        void SetAggregationEnabled(bool _enabled);
        void SetAsyncEnabled(bool _enabled);

        // Lod tuning
        void SetCoreRadius(int _radius);
        void SetRingGrowth(float _growth);
        void SetMaxLevel(int _level);

        // Streaming queries
        bool  IsChunkMeshedAt(const vec3& _pos);
        bool  IsStreamingIdle() const;
        float GetCoverage() const;

        // Pure-voxel core radius, lod needs higher
        static constexpr int kDefaultCoreRadius = 16;

    private:
        // ChunkManager.cpp --------------------------------------------------------------------------------------------

        RingParams BuildRingParams() const;
        void  RebuildRingOffset();
        void  NormalizeRanges();
        void  UnloadFar(CHUNK::Coord _centre);
        float ComputeCoverage(CHUNK::Coord _centre) const;

        void DrainGenResults();
        void DrainMeshResults();

        static CHUNK::Coord WorldToChunk(const vec3& _pos);
        static int chessDist(CHUNK::Coord _chunk);

        // ChunkManagerChunks.cpp - chunk streaming --------------------------------------------------------------------

        std::shared_ptr<Chunk> MakeChunk(CHUNK::Coord _coord) const;
        void GenerateChunk(CHUNK::Coord _coord);

        int  EnsureGenerated(CHUNK::Coord _centre);
        int  EnsureMeshed(CHUNK::Coord _centre);
        void BuildChunkMesh(Chunk& _chunk);
        void UploadChunkMesh(Chunk& _chunk, ChunkMeshes&& _meshes);

        bool ChunkReadyAt(CHUNK::Coord _coord) const;
        bool NeighboursGenerated(CHUNK::Coord _coord);
        Chunk* GetChunk(CHUNK::Coord _coord);
        ChunkBorders GatherBorders(CHUNK::Coord _coord);

        // ChunkManagerTiles.cpp - Lod streaming/lifecycle -------------------------------------------------------------

        int     EnsureNodes(CHUNK::Coord _centre);
        LodTile MakeTile(const LodNodeKey& _key, int _coreMask);
        void    BuildTile(const LodNodeKey& _key, int _coreMask);
        LodTile UploadTile(int _coreMask, LodMeshResult&& _data);
        int     ComputeCoreMask(const LodNodeKey& _key, CHUNK::Coord _centre) const;

        // flip/retire pass
        void CommitFlips();
        void RetireReplaced(CHUNK::Coord _centre);
        void EraseCoveredBy(const LodNodeKey& _key);

        // tile map
        void StoreTile(const LodNodeKey& _key, LodTile&& _tile);
        void StorePending(const LodNodeKey& _key, LodTile&& _tile);
        auto EraseTile(TileMap::iterator _it) -> TileMap::iterator;
        auto ErasePending(TileMap::iterator _it) -> TileMap::iterator;
        void ActivatePending(const LodNodeKey& _key);

        // tile & node queries
        bool TileCoveringReady(CHUNK::Coord _coord) const;
        bool AnyTileAt(CHUNK::Coord _coord) const;
        bool ReadyTileAt(CHUNK::Coord _coord) const;
        bool ReadyNodeAt(CHUNK::Coord _origin, int _level, int _footprint) const;
        bool HasLiveAncestor(const LodNodeKey& _key) const;
        bool CoveredByReplacement(CHUNK::Coord _origin, int _level) const;
        bool AreaCovered(const LodNodeKey& _key) const;

        // DATA --------------------------------------------------------------------------------------------------------

        // injected algorithms
        ChunkGenerator m_generator;
        LodMesher      m_lodMesher;

        // mats
        std::shared_ptr<Material> m_blockMat;
        std::shared_ptr<Material> m_vegMat;

        // chunk streaming state
        ChunkMap m_chunks;

        // ring scan offsets
        std::vector<CHUNK::Coord> m_genOffsets;
        int m_offsetsBuiltForRadius = -1;

        // lod tile lifecycle state
        TileMap m_lodTiles;
        TileMap m_pendingTiles;
        NodeSet m_desiredSet;
        NodeSet m_liveKeys;
        std::vector<LodNodeKey>   m_desiredKeys;
        std::vector<PendingBuild> m_buildQueue;
        bool m_desiredFresh = false;

        // streaming status
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

        // async MT
        bool   m_asyncEnabled = false;
        uInt64 m_epoch = 0; // time stamp for workers

        // Chunk off thread generation
        std::unordered_set<CHUNK::Coord, CHUNK::CoordHash> m_genInFlight;
        std::mutex                                         m_resultMutex;
        std::vector<GenResult>                             m_genResults;
        // Chunk of thread meshing
        std::unordered_set<CHUNK::Coord, CHUNK::CoordHash> m_meshInFlight;
        std::vector<MeshResult>                            m_meshResults;

        std::unique_ptr<WorkerPool> m_pool;

        // Per frame budgets
        static constexpr int kGenBudget  = 1;
        static constexpr int kMeshBudget = 1;
        static constexpr int kTileBudget = 4;

        // Per frame thread budgets
        static constexpr float kInFlightWorkerMultiplier = 2.0f;
        // GL uploads per frame
        static constexpr int kUploadBudget = 4;
    };
}
