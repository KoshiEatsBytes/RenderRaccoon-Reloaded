
#pragma once
#include <vector>

#include "Helpers/Types.h"

namespace RR
{
    // VRAM memory allocator
    class GPUBufferAllocator
    {
    public:
        static constexpr uInt64 kInvalid = ~0ull;

        explicit GPUBufferAllocator(uInt64 _capacity = 0);

        // buffer lifecycle
        uInt64 Allocate(uInt64 _count);
        void Free(uInt64 _offset, uInt64 _count);
        void Grow(uInt64 _newCapacity);
        void Reset();

        // getters
        uInt64 GetCapacity()   const;
        uInt64 GetCeiling()    const;
        uInt64 GetUnitsFree()  const;
        uInt64 GetUnitsUsed()  const;
        sizeT  GetFreeBlocks() const;

    private:
        struct FreeBlock
        {
            uInt64 offset;
            uInt64 count;
        };

        // pull to highest blocks
        void BumpCeiling();

        std::vector<FreeBlock> m_free;
        uInt64 m_capacity  = 0;
        uInt64 m_ceiling   = 0;
        uInt64 m_unitsFree = 0;
    };
}
