#include "runtime.h"

#include <cstring>

#include "cuda/cuda_platform.h"
#include "dummy_platform.h"
#include "host/cpu_platform.h"

namespace AnyDSLInternal {
Runtime::Runtime()
    : mHost(nullptr)
{
}

Runtime::~Runtime()
{
}

void register_cpu_platform(Runtime* runtime)
{
    runtime->register_platform<CpuPlatform>();
}

#ifndef AnyDSL_runtime_HAS_CUDA_SUPPORT
void register_cuda_platform(Runtime* runtime)
{
    runtime->register_platform<DummyPlatform>("CUDA", AnyDSL_DEVICE_CUDA);
}
#else
void register_cuda_platform(Runtime* runtime)
{
    runtime->register_platform<CudaPlatform>();
}
#endif

#ifndef AnyDSL_runtime_HAS_OPENCL_SUPPORT
void register_opencl_platform(Runtime* runtime)
{
    runtime->register_platform<DummyPlatform>("OpenCL", AnyDSL_DEVICE_OPENCL);
}
#endif

#ifndef AnyDSL_runtime_HAS_HSA_SUPPORT
void register_hsa_platform(Runtime* runtime)
{
    runtime->register_platform<DummyPlatform>("HSA", AnyDSL_DEVICE_HSA);
}
#endif

struct RuntimeSingleton {
    Runtime runtime;

    RuntimeSingleton()
        : runtime()
    {
        register_cpu_platform(&runtime);
        register_cuda_platform(&runtime);
        register_opencl_platform(&runtime);
        register_hsa_platform(&runtime);

        runtime.init();
    }
};

Runtime& Runtime::instance()
{
    static RuntimeSingleton singleton;
    return singleton.runtime;
}

void Runtime::init()
{
    for (size_t i = 0; i < mPlatforms.size(); ++i) {
        AnyDSLResult res = mPlatforms[i]->init();

        if (res != AnyDSL_SUCCESS && res != AnyDSL_NOT_AVAILABLE)
            error("Could not init platform %s", mPlatforms[i]->name().c_str());
    }
}

std::optional<Platform*> Runtime::query_platform(AnyDSLDeviceType type)
{
    for (size_t i = 0; i < mPlatforms.size(); ++i) {
        if (mPlatforms.at(i)->type() == type)
            return mPlatforms.at(i).get();
    }

    return std::nullopt;
}

#if _POSIX_VERSION >= 200112L || _XOPEN_SOURCE >= 600
void* Runtime::aligned_malloc(size_t size, size_t alignment)
{
    void* p = nullptr;
    if (posix_memalign(&p, alignment, size) != 0)
        return nullptr;
    return p;
}
void Runtime::aligned_free(void* ptr)
{
    free(ptr);
}
#elif _ISOC11_SOURCE
void* Runtime::aligned_malloc(size_t size, size_t alignment)
{
    return ::aligned_alloc(alignment, size);
}
void Runtime::aligned_free(void* ptr)
{
    ::free(ptr);
}
#elif defined(_WIN32) || defined(__CYGWIN__)
#include <malloc.h>
void* Runtime::aligned_malloc(size_t size, size_t alignment)
{
    return ::_aligned_malloc(size, alignment);
}
void Runtime::aligned_free(void* ptr)
{
    ::_aligned_free(ptr);
}
#else
#error "There is no way to allocate aligned memory on this system"
#endif
} // namespace AnyDSLInternal