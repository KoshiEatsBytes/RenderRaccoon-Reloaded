
#pragma once
#include <memory>
#include <unordered_map>

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

        void GenerateGrid(int _radius);
        void SubmitDraws();

    private:
        void GenerateChunk(CHUNK::Coord _coord);
        void BuildChunkMesh(Chunk& _chunk);

        Chunk* GetChunk(CHUNK::Coord _coord);
        ChunkBorders GatherBorders(CHUNK::Coord _coord);

        std::unordered_map<CHUNK::Coord, std::unique_ptr<Chunk>, CHUNK::CoordHash> m_chunks;
        std::shared_ptr<Material> m_material;
        ChunkGenerator            m_generator;
    };
}