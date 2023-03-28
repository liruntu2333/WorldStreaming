#pragma once
#include <memory>

class ThreadPool;
struct GlobalContext
{
    std::unique_ptr<ThreadPool> Pool = nullptr;
    size_t ThreadCount = 0;

    void Initialize(size_t threadCount);
};

extern GlobalContext g_Context;
