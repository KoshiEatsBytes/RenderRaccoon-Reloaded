
#pragma once
#include <GL/glew.h>
#include <vector>

#include "Types.h"

namespace RR
{
    struct VertexElement
    {
        GLuint index;    // Attribute location
        GLuint size;     // Number of components
        GLuint type;     // Data type (e.g. GL_UINT)
        uint32 offset;   // Bytes offset from start of vertex

        // Index for shaders
        static constexpr int PositionIndex = 0;
        static constexpr int ColorIndex = 1;
        static constexpr int UVIndex = 2;
        static constexpr int NormalIndex = 3;
    };

    struct VertexLayout
    {
        std::vector<VertexElement> elements;
        uint32 stride = 0; // Total size of a single vertex
    };
}