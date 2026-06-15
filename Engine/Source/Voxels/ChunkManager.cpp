
#include "ChunkManager.h"

#include "Render/Mesh.h"
#include "Render/RenderQueue.h"
#include "Render/Voxels/ChunkMesher.h"
#include "Render/Material.h"
#include "Engine.h"
#include <glm/gtc/matrix_transform.hpp>

namespace RR
{
    // PUBLIC ----------------------------------------------------------------------------------------------------------

    ChunkManager::ChunkManager(ChunkGenerator _generator, std::shared_ptr<Material> _material)
        : m_material(std::move(_material)), m_generator(std::move(_generator))
    {
    }

    ChunkManager::~ChunkManager()
    = default;

    // Generate and mesh radius of chunk around the camera,
    // this is effectively the "render distacne"
    void ChunkManager::GenerateGrid(int _radius)
    {
        // generate borders before meshing
        for (int z = _radius; z <= _radius; z++)
        {
            for (int x = -_radius; x < _radius; x++)
            {
                // Nothing to check if the chunk has already been generated?
                GenerateChunk({x, z});
            }
        }

        // Mesh
        for (auto& [coord, chunk] : m_chunks)
        {
            BuildChunkMesh(*chunk);
        }
    }

    void ChunkManager::SubmitDraws()
    {
        auto& queue = Engine::GetInstance().GetRenderQueue();

        for (auto& [coord, chunk] : m_chunks)
        {
            if (!chunk->mesh) continue;

            auto chunkMatrix = glm::translate(mat4(1.0f),
                vec3(coord.x * CHUNK::kSizeX, 0.0f, coord.z * CHUNK::kSizeZ));;

            // Create and submit render command for that chunk
            RenderCommand command;
            command.material    = m_material.get();
            command.mesh        = chunk->mesh.get();
            command.modelMatrix = chunkMatrix;
            command.color       = vec3(1.0f);

            queue.Submit(command);
        }
    }

    // PRIVATE ---------------------------------------------------------------------------------------------------------

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
        // For now all air
        const ChunkBorders air {};

        MeshData data = MeshChunk(_chunk, air);

        _chunk.mesh  = std::make_unique<Mesh>(data.layout, data.vertices, data.indices);
        _chunk.state = CHUNK::STATE::MESHED;
    }
}
