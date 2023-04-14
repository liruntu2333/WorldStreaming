#pragma once
#include <memory>

class ThreadPool;
struct GlobalContext
{
    std::unique_ptr<ThreadPool> Pool = nullptr;
    size_t ThreadCount = 0;

    void Initialize(size_t threadCount);
};

constexpr uint32_t OBJECT_MAX = 1 << 14;

extern GlobalContext g_Context;
