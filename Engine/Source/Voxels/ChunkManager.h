
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

namespace RR
{
    // lod mesh data for render
    struct LodMeshResult
    {
        MeshData surface;
        MeshData proxies;
    };

    using LodMesher = std::function<LodMeshResult(CHUNK::Coord, int)>;

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

        void SetFancyLeaves(bool _fancyLeaves);
        bool GetFancyLeaves() const;

        void SetRenderDistance(int _distance);
        int  GetRenderDistance() const;

        // Lod tuning
        void SetLodEnabled(bool _enabled);
        void SetCoreRadius(int _radius);
        void SetRingGrowth(float _growth);
        void SetMaxLevel(int _level);

        // Pure-voxel core radius, lod needs higher
        static constexpr int kDefaultCoreRadius = 16;

        bool IsChunkMeshedAt(const vec3& _pos);
        bool IsStreamingIdle() const;
        float GetCoverage() const;

    private:
        void RebuildRingOffset();
        void NormalizeRanges();
        void UnloadFar(CHUNK::Coord _centre);
        void RetireReplaced(CHUNK::Coord _centre);
        void GenerateChunk(CHUNK::Coord _coord);
        void BuildChunkMesh(Chunk& _chunk);

        int   EnsureGenerated(CHUNK::Coord _centre);
        int   EnsureMeshed(CHUNK::Coord _centre);
        int   EnsureTiles(CHUNK::Coord _centre);
        bool  NeighboursGenerated(CHUNK::Coord _coord);
        float ComputeCoverage(CHUNK::Coord _centre) const;

        int LevelForDistance(int _dist) const;

        Chunk* GetChunk(CHUNK::Coord _coord);
        ChunkBorders GatherBorders(CHUNK::Coord _coord);
        static CHUNK::Coord WorldToChunk(const vec3& _pos);

        // Algorithms
        ChunkGenerator m_generator;
        LodMesher      m_lodMesher;

        // terrain/visual
        std::unordered_map<CHUNK::Coord, std::unique_ptr<Chunk>, CHUNK::CoordHash> m_chunks;
        std::unordered_map<CHUNK::Coord, LodTile, CHUNK::CoordHash>                m_lodTiles;

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
        bool m_fancyLeaves = true;
        bool m_lodEnabled  = false;

        // RD settings
        int m_meshRadius = 16; // total RD
        int m_coreRadius = kDefaultCoreRadius; // full detail RD

        // Lod ring shaping
        float m_ringGrowth    = 2.f;
        int   m_maxLevel      = 4;
        int   m_lodHysteresis = 2; // chunks of deadband around each ring

        // Per frame budget
        static constexpr int kGenBudget  = 1;
        static constexpr int kMeshBudget = 1;

        // lod rings and budgets
        static constexpr int kTileBudget  = 2;
    };
}