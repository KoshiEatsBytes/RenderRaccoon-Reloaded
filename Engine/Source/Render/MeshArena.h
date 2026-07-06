
#pragma once
#include <vector>
#include <GL/glew.h>

#include "Helpers/Types.h"
#include "Graphics/VertexLayout.h"
#include "Render/GPUBufferAllocator.h"

namespace RR
{
    // single VBO + EBO + VAO tjat every voxel mesh is allocated into
    // collapses many draw calls into 1
    class MeshArena
    {
    public:
        // holds and call this mesh arena on closedown
        struct ArenaHandler
        {
            uInt32 baseVertex  = 0;   // vertex offset
            uInt32 vertexCount = 0;
            uInt32 firstIndex  = 0;   // index offset
            uInt32 indexCount  = 0;
            bool valid = false;
        };

        MeshArena(VertexLayout _layout, uInt64 _initialVertices, uInt64 _initialIndices);
        ~MeshArena();

        // no copies, otherwise GL explodes
        MeshArena(const MeshArena&)            = delete;
        MeshArena& operator=(const MeshArena&) = delete;

        ArenaHandler Upload(const std::vector<float>& _vertices, const std::vector<uInt32>& _indices);

        void HandleRange(const ArenaHandler& _slice);
        void Reset();
        void Reserve(uInt64 _verts, uInt64 _indices);
        void Draw(const std::vector<const ArenaHandler*>& _visible) const;

        // diagnostics
        uInt64 GetVerticesUsed() const;
        uInt64 GetIndicesUsed() const;
        uInt64 GetVertexCapacity() const;
        sizeT  GetVertexFragments() const;

    private:
        void ConfigureVao();
        void GrowVerticesTo(uInt64 _newSize);
        void GrowIndicesTo(uInt64 _newSize);
        void GrowVertices(uInt64 _request);
        void GrowIndices(uInt64 _request);

        VertexLayout m_layout;
        uInt32 m_floatsPerVertex = 0; // stride

        GLuint m_vbo = 0;
        GLuint m_ebo = 0;
        GLuint m_vao = 0;

        uInt64 m_maxVertices = 0;
        uInt64 m_maxIndices  = 0;

        GPUBufferAllocator m_vtxAlloc;
        GPUBufferAllocator m_idxAlloc;

        mutable std::vector<GLsizei> m_drawCounts;
        mutable std::vector<void*>   m_drawOffsets;
        mutable std::vector<GLint>   m_drawBase;
    };
}
