
#pragma once
#include "MeshArena.h"

namespace RR
{
    // owns a mesh of a shared MeshArena and hands it back for destruction
    class PooledMesh
    {
    public:
        PooledMesh() = default;

        // Upload handle to a slice from arena
        PooledMesh(MeshArena* _arena, const MeshArena::ArenaHandler& _handle)
            : m_arena(_arena), m_handle(_handle) {}

        // return slice
        ~PooledMesh()
        {
            Release();
        }

        // Move only
        PooledMesh(const PooledMesh&) = delete;
        PooledMesh& operator=(const PooledMesh&) = delete;

        // handle move from anoterh pool
        PooledMesh(PooledMesh&& _other) noexcept
        {
            TakeFrom(_other);
        }

        PooledMesh& operator=(PooledMesh&& _other) noexcept
        {
            if (this != &_other)
            {
                // let go of this mesh memory, and move it to the receiver
                Release();
                TakeFrom(_other);
            }
            return *this;
        }

        // The slice to draw
        const MeshArena::ArenaHandler& GetHandle() const
        {
            return m_handle;
        }

    private:
        // Steal other pool slice and leave it empty
        void TakeFrom(PooledMesh& _other)
        {
            m_arena  = _other.m_arena;
            m_handle = _other.m_handle;

            _other.m_arena  = nullptr;
            _other.m_handle = {};
        }

        // Returns slice to the arena
        void Release()
        {
            if (m_arena && m_handle.valid)
            {
                m_arena->HandleRange(m_handle);
            }

            m_arena  = nullptr;
            m_handle = {};
        }

        MeshArena* m_arena = nullptr;
        MeshArena::ArenaHandler m_handle;
    };
}
