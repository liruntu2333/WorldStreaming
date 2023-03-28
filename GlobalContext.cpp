#include "GlobalContext.h"
#include "ThreadPool.h"

GlobalContext g_Context;

void GlobalContext::Initialize(size_t threadCount)
{
    Pool = std::make_unique<ThreadPool>(threadCount);
    ThreadCount = threadCount;
}
