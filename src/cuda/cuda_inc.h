#pragma once

#include "anydsl_runtime_config.h"
#include "anydsl_runtime_internal_config.h"

#include "log.h"

#include <string>

#define CUDA_API_PER_THREAD_DEFAULT_STREAM
#include <cuda.h>
#include <nvvm.h>

#if CUDA_VERSION < 6050
#error "CUDA 6.5 or higher required!"
#endif

namespace AnyDSLInternal {
#define CHECK_NVVM(err, name) \
    AnyDSLInternal::check_nvvm_errors(err, name, __FILE__, __LINE__)
#define CHECK_CUDA(err, name) \
    AnyDSLInternal::check_cuda_errors(err, name, __FILE__, __LINE__)

#define CHECK_NVVM_RET(err, name)                                \
    if (auto res = CHECK_NVVM(err, name); res != AnyDSL_SUCCESS) \
    return res
#define CHECK_CUDA_RET(err, name)                                \
    if (auto res = CHECK_CUDA(err, name); res != AnyDSL_SUCCESS) \
    return res

inline AnyDSLResult check_cuda_errors(CUresult err, const char* name, const char* file, const int line)
{
    if (CUDA_SUCCESS != err) {
        if (err == CUDA_ERROR_OUT_OF_MEMORY)
            return AnyDSL_OUT_OF_DEVICE_MEMORY;

        const char* error_name;
        const char* error_string;
        cuGetErrorName(err, &error_name);
        cuGetErrorString(err, &error_string);
        auto msg = std::string(error_name) + ": " + std::string(error_string);
        error("Driver API function %s (%i) [file %s, line %i]: %s", name, (int)err, file, line, msg.c_str());
        return AnyDSL_PLATFORM_ERROR;
    }

    return AnyDSL_SUCCESS;
}

inline AnyDSLResult check_nvvm_errors(nvvmResult err, const char* name, const char* file, const int line)
{
    if (NVVM_SUCCESS != err) {
        error("NVVM API function %s (%i) [file %s, line %i]: %s", name, (int)err, file, line, nvvmGetErrorString(err));
        return AnyDSL_PLATFORM_ERROR;
    }

    return AnyDSL_SUCCESS;
}
} // namespace AnyDSLInternal