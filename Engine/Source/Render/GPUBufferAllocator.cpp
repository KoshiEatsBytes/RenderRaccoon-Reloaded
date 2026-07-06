
#include "GPUBufferAllocator.h"

namespace RR
{
    GPUBufferAllocator::GPUBufferAllocator(uInt64 _capacity)
        : m_capacity(_capacity), m_ceiling(0), m_unitsFree(_capacity) {}

    // Reserve specified units
    uInt64 GPUBufferAllocator::Allocate(uInt64 _count)
    {
        // zero units dont touch buffer, return ivalid
        if (_count == 0) return kInvalid;

        // best fit is smallest block that'd fit
        sizeT  best      = static_cast<sizeT>(-1);
        uInt64 bestCount = kInvalid;

        for (sizeT i = 0; i < m_free.size(); ++i)
        {
            if (m_free[i].count >= _count && m_free[i].count < bestCount)
            {
                best = i;
                bestCount = m_free[i].count;

                // exact fit, return instant, cant be beter
                if (bestCount == _count)
                    break;
            }
        }

        // if a best is found
        if (best != static_cast<sizeT>(-1))
        {
            const uInt64 offset = m_free[best].offset;

            if (m_free[best].count == _count)
            {
                m_free.erase(m_free.begin() + static_cast<long long>(best));
            }
            else
            {
                // carve from front, try leave rest in place
                m_free[best].offset += _count;
                m_free[best].count  -= _count;
            }

            m_unitsFree -= _count;
            return offset;
        }

        // No free block, bump ceiling up to allocate
        if (m_ceiling + _count <= m_capacity)
        {
            const uInt64 offset = m_ceiling;
            m_ceiling   += _count;
            m_unitsFree -= _count;

            return offset;
        }

        // if failed is invalid
        return kInvalid;
    }

    // Free and return what was previously allocated
    void GPUBufferAllocator::Free(uInt64 _offset, uInt64 _count)
    {
        if (_count == 0) return;

        m_unitsFree += _count;

        // if topmost retract ceiling
        if (_offset + _count == m_ceiling)
        {
            m_ceiling = _offset;
            BumpCeiling();
            return;
        }

        // Find first block whos offest first the new entry
        sizeT i = 0;
        while (i < m_free.size() && m_free[i].offset < _offset)
        {
            ++i;
        }

        // check if its touch a previous or next block
        const bool touchPrev = i > 0 && m_free[i - 1].offset + m_free[i - 1].count == _offset;
        const bool touchNext = i < m_free.size() && _offset + _count == m_free[i].offset;

        // if so allocate in the middle
        if (touchPrev && touchNext)
        {
            m_free[i - 1].count += _count + m_free[i].count;
            m_free.erase(m_free.begin() + static_cast<long long>(i));
        }
        else if (touchPrev)
        {
            m_free[i - 1].count += _count;
        }
        else if (touchNext)
        {
            m_free[i].offset  = _offset;
            m_free[i].count  += _count;
        }
        else
        {
            m_free.insert(m_free.begin() + static_cast<long long>(i), {_offset, _count});
        }

        // updated block might now bump ceiling up, update if so
        BumpCeiling();
    }

    void GPUBufferAllocator::BumpCeiling()
    {
        while (!m_free.empty() &&
               m_free.back().offset + m_free.back().count == m_ceiling)
        {
            m_ceiling = m_free.back().offset;
            m_free.pop_back();
        }
    }

    // increase total capacity
    void GPUBufferAllocator::Grow(uInt64 _newCapacity)
    {
        if (_newCapacity <= m_capacity) return;

        m_unitsFree += (_newCapacity - m_capacity);
        m_capacity   = _newCapacity;
    }

    // Drop every allocation, keep capacity
    void GPUBufferAllocator::Reset()
    {
        m_free.clear();
        m_ceiling = 0;
        m_unitsFree = m_capacity;
    }

    uInt64 GPUBufferAllocator::GetCapacity() const
    {
        return m_capacity;
    }

    uInt64 GPUBufferAllocator::GetCeiling() const
    {
        return m_ceiling;
    }

    uInt64 GPUBufferAllocator::GetUnitsFree() const
    {
        return m_unitsFree;
    }

    uInt64 GPUBufferAllocator::GetUnitsUsed() const
    {
        return m_capacity - m_unitsFree;
    }

    sizeT GPUBufferAllocator::GetFreeBlocks() const
    {
        return m_free.size();
    }
}
