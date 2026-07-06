
#include "Chunk.h"
#include "Render/PooledMesh.hpp"

// this entire translation unit exists because CPP is a ******* ******
namespace RR
{
    Chunk::Chunk(CHUNK::Coord _coord)
        : coord(_coord)
    {}

    Chunk::~Chunk()
    = default;
}

