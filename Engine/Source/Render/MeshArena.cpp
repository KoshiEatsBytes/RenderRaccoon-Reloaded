
#include "MeshArena.h"

#include <algorithm>

#include "Helpers/Printer.hpp"

namespace RR
{
    MeshArena::MeshArena(VertexLayout _layout, uInt64 _initialVertices, uInt64 _initialIndices)
        : m_layout(std::move(_layout)),
          m_maxVertices(_initialVertices),
          m_maxIndices(_initialIndices),
          m_vtxAlloc(_initialVertices),
          m_idxAlloc(_initialIndices)
    {
        m_floatsPerVertex = m_layout.stride / static_cast<uInt32>(sizeof(float));

        glGenBuffers(1, &m_vbo);
        glGenBuffers(1, &m_ebo);
        glGenVertexArrays(1, &m_vao);

        // Preallocate gpu memory up-front
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(m_maxVertices * m_layout.stride), nullptr, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(m_maxIndices * sizeof(uInt32)), nullptr, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        ConfigureVao();
    }

    MeshArena::~MeshArena()
    {
        // destroy gl buffers
        glDeleteBuffers(1, &m_ebo);
        glDeleteBuffers(1, &m_vbo);
        glDeleteVertexArrays(1, &m_vao);
    }

    // re-uploads params to current VAO
    void MeshArena::ConfigureVao()
    {
        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

        for (const auto& element : m_layout.elements)
        {
            // attrib pointer
            glVertexAttribPointer(
                element.index,
                static_cast<GLint>(element.size),
                element.type,
                GL_FALSE,
                static_cast<GLsizei>(m_layout.stride),
                reinterpret_cast<void*>(static_cast<uintptr_t>(element.offset))
                );

            glEnableVertexAttribArray(element.index);
        }

        // bind buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);

        // after bind success unload
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    // copy mesh vertices into its bind arena, return invalid if handle empty
    MeshArena::ArenaHandler MeshArena::Upload(const std::vector<float>& _vertices,
        const std::vector<uInt32>& _indices)
    {
        ArenaHandler slice;

        const uInt64 vertCount = _vertices.size() / m_floatsPerVertex;
        const uInt64 idxCount  = _indices.size();

        // mesh is empty, discard
        if (vertCount == 0 || idxCount == 0) return slice;

        // allocate vertex vector
        uInt64 vertOffset = m_vtxAlloc.Allocate(vertCount);
        if (vertOffset == GPUBufferAllocator::kInvalid)
        {
            GrowVertices(vertCount);
            vertOffset = m_vtxAlloc.Allocate(vertCount);
        }

        uInt64 idxOffset = m_idxAlloc.Allocate(idxCount);
        if (idxOffset == GPUBufferAllocator::kInvalid)
        {
            GrowIndices(idxCount);
            idxOffset = m_idxAlloc.Allocate(idxCount);
        }

        // UNBIND before drawing otherwise it overrides last
        glBindVertexArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferSubData(
            GL_ARRAY_BUFFER,
            static_cast<GLintptr>(vertOffset * m_layout.stride),
            static_cast<GLsizeiptr>(_vertices.size() * sizeof(float)),
            _vertices.data()
            );

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
        glBufferSubData(
            GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLintptr>(idxOffset * sizeof(uInt32)),
            static_cast<GLsizeiptr>(idxCount * sizeof(uInt32)),
            _indices.data()
            );

        // unbind after upload
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        slice.baseVertex  = static_cast<uInt32>(vertOffset);
        slice.vertexCount = static_cast<uInt32>(vertCount);
        slice.firstIndex  = static_cast<uInt32>(idxOffset);
        slice.indexCount  = static_cast<uInt32>(idxCount);
        slice.valid       = true;
        return slice;
    }

    // Return a handle range to allocators
    void MeshArena::HandleRange(const ArenaHandler& _slice)
    {
        if (!_slice.valid) return;

        m_vtxAlloc.Free(_slice.baseVertex, _slice.vertexCount);
        m_idxAlloc.Free(_slice.firstIndex, _slice.indexCount);
    }

    // Clear every slice
    void MeshArena::Reset()
    {
        m_vtxAlloc.Reset();
        m_idxAlloc.Reset();
    }

    // Bind the shared VAO, then issue one multidraw
    void MeshArena::Draw(const std::vector<const ArenaHandler*>& _visible) const
    {
        m_drawCounts.clear();
        m_drawOffsets.clear();
        m_drawBase.clear();

        for (const ArenaHandler* handle : _visible)
        {
            if (!handle || !handle->valid || handle->indexCount == 0) continue;

            // push visible areas (handles) for draw
            m_drawCounts.push_back(static_cast<GLsizei>(handle->indexCount));
            m_drawOffsets.push_back(reinterpret_cast<void*>(static_cast<uintptr_t>(handle->firstIndex * sizeof(uInt32))));
            m_drawBase.push_back(static_cast<GLint>(handle->baseVertex));
        }

        // nothing to draw, discard
        if (m_drawCounts.empty()) return;

        // bind vertex for correct draw
        glBindVertexArray(m_vao);
        glMultiDrawElementsBaseVertex(
            GL_TRIANGLES,
            m_drawCounts.data(),
            GL_UNSIGNED_INT,
            m_drawOffsets.data(),
            static_cast<GLsizei>(m_drawCounts.size()),
            m_drawBase.data()
            );
    }

    uInt64 MeshArena::GetVerticesUsed() const
    {
        return m_vtxAlloc.GetUnitsUsed();
    }

    uInt64 MeshArena::GetIndicesUsed() const
    {
        return m_idxAlloc.GetUnitsUsed();
    }

    uInt64 MeshArena::GetVertexCapacity() const
    {
        return m_maxVertices;
    }

    sizeT MeshArena::GetVertexFragments() const
    {
        return m_vtxAlloc.GetFreeBlocks();
    }

    void MeshArena::Reserve(uInt64 _verts, uInt64 _indices)
    {
        GrowVerticesTo(_verts);
        GrowIndicesTo(_indices);
    }

    // grow to an exact capacity
    void MeshArena::GrowVerticesTo(uInt64 _newSize)
    {
        // if asked to shrink discard
        if (_newSize <= m_maxVertices) return;

        GLuint newVbo = 0;
        glGenBuffers(1, &newVbo);
        glBindBuffer(GL_ARRAY_BUFFER, newVbo);
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(_newSize * m_layout.stride),
            nullptr,
            GL_DYNAMIC_DRAW
            );

        const uInt64 liveBytes = m_vtxAlloc.GetCeiling() * m_layout.stride;
        if (liveBytes > 0)
        {
            glBindBuffer(GL_COPY_READ_BUFFER, m_vbo);
            glBindBuffer(GL_COPY_WRITE_BUFFER, newVbo);

            glCopyBufferSubData(
                GL_COPY_READ_BUFFER,
                GL_COPY_WRITE_BUFFER,
                0, 0,
                static_cast<GLsizeiptr>(liveBytes)
                );

            glBindBuffer(GL_COPY_READ_BUFFER, 0);
            glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
        }

        glDeleteBuffers(1, &m_vbo);
        m_vbo         = newVbo;
        m_maxVertices = _newSize;
        m_vtxAlloc.Grow(_newSize);
        ConfigureVao();

        Warn("[MESHARENA] vertex arena sized to '", _newSize, "' verts (",
             _newSize * m_layout.stride / (1024 * 1024), " MB)");
    }

    void MeshArena::GrowIndicesTo(uInt64 _newSize)
    {
        // if asked to shrink discard
        if (_newSize <= m_maxIndices) return;

        GLuint newEbo = 0;
        glGenBuffers(1, &newEbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, newEbo);

        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(_newSize * sizeof(uInt32)),
            nullptr,
            GL_DYNAMIC_DRAW
            );

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        const uInt64 liveBytes = m_idxAlloc.GetCeiling() * sizeof(uInt32);
        if (liveBytes > 0)
        {
            glBindBuffer(GL_COPY_READ_BUFFER, m_ebo);
            glBindBuffer(GL_COPY_WRITE_BUFFER, newEbo);
            glCopyBufferSubData(
                GL_COPY_READ_BUFFER,
                GL_COPY_WRITE_BUFFER,
                0, 0,
                static_cast<GLsizeiptr>(liveBytes)
                );

            glBindBuffer(GL_COPY_READ_BUFFER, 0);
            glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
        }

        glDeleteBuffers(1, &m_ebo);
        m_ebo        = newEbo;
        m_maxIndices = _newSize;
        m_idxAlloc.Grow(_newSize);
        ConfigureVao();

        Warn("[MESHARENA] index arena sized to '", _newSize, "' indices");
    }

    void MeshArena::GrowVertices(uInt64 _request)
    {
        GrowVerticesTo(std::max(m_maxVertices * 2, m_vtxAlloc.GetCeiling() + _request));
    }

    void MeshArena::GrowIndices(uInt64 _request)
    {
        GrowIndicesTo(std::max(m_maxIndices * 2, m_idxAlloc.GetCeiling() + _request));
    }
}
