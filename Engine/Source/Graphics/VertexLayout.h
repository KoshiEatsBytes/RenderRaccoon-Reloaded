
#pragma once
#include <GL/glew.h>
#include <vector>

#include "../Helpers/Types.h"

namespace RR
{
    struct VertexElement
    {
        GLuint index;    // Attribute location
        GLuint size;     // Number of components
        GLuint type;     // Data type (e.g. GL_UINT)
        uInt32 offset;   // Bytes offset from start of vertex

        // NOTE: these describe the DEFAULT/Box vertex format (pos/color/uv/normal).
        // They are NOT a global slot map — the voxel format reuses slots 1 & 3 for normal & layer.
        static constexpr int PositionIndex = 0;
        static constexpr int ColorIndex    = 1;
        static constexpr int UVIndex       = 2;
        static constexpr int NormalIndex   = 3;
    };

    struct VertexLayout
    {
        std::vector<VertexElement> elements;
        uInt32 stride = 0; // Total size of a single vertex
    };
}