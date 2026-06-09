
#pragma once
#include <vector>
#include <cstdint>

#include "Graphics/VertexLayout.h"

namespace RR
{
    // Mesh owning data
    struct MeshData
    {
        bool Empty() const
        {
            return vertices.empty();
        }

        VertexLayout          layout;
        std::vector<float>    vertices;
        std::vector<uint32_t> indices;
    };
}