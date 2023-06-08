#ifndef ANYDSL_RUNTIME_CUDA_H
#define ANYDSL_RUNTIME_CUDA_H

#ifndef ANYDSL_RUNTIME_H
#error This header should only be included by anydsl_runtime.h
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct AnyDSLDeviceOptionsCuda {
    AnyDSLStructureType sType;
    const void* pNext;

    AnyDSLBool dumpCubin; // If true, will dump cubin into the cache directory. Default is false
} AnyDSLDeviceOptionsCuda;

typedef struct AnyDSLDeviceFeaturesCuda {
    AnyDSLStructureType sType;
    const void* pNext;

    size_t freeMemory;

    size_t maxThreadsPerBlock;
    size_t maxBlockDim[3];
    size_t maxGridDim[3];
    size_t maxSharedMemPerBlock; // In bytes
    size_t maxRegistersPerBlock;
} AnyDSLDeviceFeaturesCuda;

#ifdef __cplusplus
}
#endif
#endif