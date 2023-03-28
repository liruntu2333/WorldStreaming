#pragma once

#include <vector>
#include <xsimd/xsimd.hpp>

template <class T, size_t Capacity, size_t Alignment>
class AlignedVector
{
public:
    AlignedVector(size_t row) : m_Data(Capacity * row), m_RowCount(row) {}
    ~AlignedVector() = default;

    AlignedVector(const AlignedVector& other) = delete;
    AlignedVector(AlignedVector&&) = delete;
    AlignedVector& operator=(const AlignedVector&) = delete;
    AlignedVector& operator=(AlignedVector&&) = delete;

    T* operator[](size_t i)
    {
        return m_Data.data() + i * Capacity;
    }

private:
    using Allocator = xsimd::aligned_allocator<T, Alignment>;
    std::vector<T, Allocator> m_Data;
    const size_t m_RowCount;
};

template <size_t Capacity, size_t Alignment>
class AlignedVector<bool, Capacity, Alignment>
{
public:
    AlignedVector(size_t row);
    ~AlignedVector();

    AlignedVector(const AlignedVector& other) = delete;
    AlignedVector(AlignedVector&&) = delete;
    AlignedVector& operator=(const AlignedVector&) = delete;
    AlignedVector& operator=(AlignedVector&&) = delete;

    bool* operator[](size_t i) const
    {
        return m_Data + i * Capacity;
    }

private:
    using Allocator = xsimd::aligned_allocator<bool, Alignment>;
    std::unique_ptr<Allocator> m_Allocator;
    bool* m_Data;
    const size_t m_RowCount;
};

template <size_t Capacity, size_t Alignment>
AlignedVector<bool, Capacity, Alignment>::AlignedVector(size_t row) :
    m_Allocator(std::make_unique<Allocator>()), m_Data(m_Allocator->allocate(Capacity * row)), m_RowCount(row)
{
}

template <size_t Capacity, size_t Alignment>
AlignedVector<bool, Capacity, Alignment>::~AlignedVector()
{
    m_Allocator->deallocate(m_Data, Capacity * m_RowCount);
}
