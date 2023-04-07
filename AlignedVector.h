#pragma once

#include <vector>
#include <xsimd/xsimd.hpp>

template <class T, size_t Column, size_t Row, size_t Alignment>
class AlignedVector
{
public:
    static_assert(Column * Row * sizeof(T) <= 1 << 10 << 7);  // 128 KB for L1 cache
    AlignedVector() : m_Data(Column * Row) {}
    ~AlignedVector() = default;

    AlignedVector(const AlignedVector& other) = delete;
    AlignedVector(AlignedVector&&) = delete;
    AlignedVector& operator=(const AlignedVector&) = delete;
    AlignedVector& operator=(AlignedVector&&) = delete;

    T* operator[](size_t i)
    {
        return m_Data.data() + i * Column;
    }

private:
    using Allocator = xsimd::aligned_allocator<T, Alignment>;
    std::vector<T, Allocator> m_Data;
};

template <size_t Column, size_t Row, size_t Alignment>
class AlignedVector<bool, Column, Row, Alignment>
{
public:
    static_assert(Column* Row * sizeof(bool) <= 1 << 17);
    AlignedVector();
    ~AlignedVector();

    AlignedVector(const AlignedVector& other) = delete;
    AlignedVector(AlignedVector&&) = delete;
    AlignedVector& operator=(const AlignedVector&) = delete;
    AlignedVector& operator=(AlignedVector&&) = delete;

    bool* operator[](size_t i) const
    {
        return m_Data + i * Column;
    }

private:
    using Allocator = xsimd::aligned_allocator<bool, Alignment>;
    std::unique_ptr<Allocator> m_Allocator;
    bool* m_Data;
};

template <size_t Column, size_t Row, size_t Alignment>
AlignedVector<bool, Column, Row, Alignment>::AlignedVector() :
    m_Allocator(std::make_unique<Allocator>()), m_Data(m_Allocator->allocate(Column* Row))
{
}

template <size_t Column, size_t Row, size_t Alignment>
AlignedVector<bool, Column, Row, Alignment>::~AlignedVector()
{
    m_Allocator->deallocate(m_Data, Column * Row);
}
