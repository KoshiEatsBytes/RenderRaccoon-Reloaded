
#pragma once
#include <memory>
#include <unordered_map>

#include "Helpers/Types.h"
#include "Voxels/Chunk.h"
#include "Voxels/ChunkData.h"
#include "Render/Voxels/ChunkMesher.h"

namespace RR
{
    class Material;
    class ChunkManager
    {
    public:
        ChunkManager(ChunkGenerator _generator, std::shared_ptr<Material> _material);
        ~ChunkManager();

        void Update(const vec3& _cameraPos);
        void SubmitDraws();

    private:
        void UnloadFar(CHUNK::Coord _centre);
        void GenerateChunk(CHUNK::Coord _coord);
        void BuildChunkMesh(Chunk& _chunk);

        void EnsureGenerated(CHUNK::Coord _centre);
        void EnsureMeshed(CHUNK::Coord _centre);
        bool NeighboursGenerated(CHUNK::Coord _coord);

        Chunk* GetChunk(CHUNK::Coord _coord);
        ChunkBorders GatherBorders(CHUNK::Coord _coord);
        static CHUNK::Coord WorldToChunk(const vec3& _pos);

        std::unordered_map<CHUNK::Coord, std::unique_ptr<Chunk>, CHUNK::CoordHash> m_chunks;
        std::shared_ptr<Material> m_material;
        ChunkGenerator            m_generator;

        CHUNK::Coord m_lastCoords;
        bool m_firstFrame = true;
        int m_meshRadius  = 12;
    };
}