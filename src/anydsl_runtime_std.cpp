#include <cassert>
#include <chrono>
#include <cmath>
#include <iostream>

#include "anydsl_runtime.h"
#include "anydsl_runtime_internal_config.h"
#include "log.h"
#include "runtime.h"

// This file contains stuff used in the artic standard library.

// TODO: Check return values!

#define AnyDSL_runtime_std_API AnyDSL_runtime_API

static inline AnyDSLDevice unwrapDevice(uint64_t id) { return (AnyDSLDevice)id; }
static inline AnyDSLBuffer unwrapBuffer(uint64_t id) { return (AnyDSLBuffer)id; }

extern "C" {
AnyDSL_runtime_std_API void anydsl_std_get_version(uint32_t* major, uint32_t* minor, uint32_t* patch)
{
    AnyDSLVersion version;
    anydslGetVersion(&version);

    *major = version.major;
    *minor = version.minor;
    *patch = version.patch;
}

AnyDSL_runtime_std_API uint64_t anydsl_std_get_device(uint32_t platform, uint32_t num)
{
    AnyDSLDevice device;

    switch (platform) {
    case 0: // Host
    {
        AnyDSLGetDeviceRequest info = {
            AnyDSL_STRUCTURE_TYPE_GET_DEVICE_REQUEST,
            nullptr,
            AnyDSL_DEVICE_HOST,
            0
        };
        anydslGetDevice(&info, &device);
    } break;
    case 1: // CUDA
    {
        AnyDSLGetDeviceRequest info = {
            AnyDSL_STRUCTURE_TYPE_GET_DEVICE_REQUEST,
            nullptr,
            AnyDSL_DEVICE_CUDA,
            num
        };
        anydslGetDevice(&info, &device);
    } break;
    case 2: // OpenCL
    {
        AnyDSLGetDeviceRequest info = {
            AnyDSL_STRUCTURE_TYPE_GET_DEVICE_REQUEST,
            nullptr,
            AnyDSL_DEVICE_OPENCL,
            num
        };
        anydslGetDevice(&info, &device);
    } break;
    case 3: // HSA
    {
        AnyDSLGetDeviceRequest info = {
            AnyDSL_STRUCTURE_TYPE_GET_DEVICE_REQUEST,
            nullptr,
            AnyDSL_DEVICE_HSA,
            num
        };
        anydslGetDevice(&info, &device);
    } break;
    default:
        anydslLogReportMessage(AnyDSL_LOG_REPORT_LEVEL_ERROR_BIT, "Invalid platform given");
        device = 0;
        break;
    }

    return (uint64_t)device;
}

AnyDSL_runtime_std_API void anydsl_std_synchronize(uint64_t deviceHandle)
{
    AnyDSLDevice device = unwrapDevice(deviceHandle);
    anydslSynchronizeDevice(device);
}

// TODO: Change this to the new interface
enum class KernelArgType : uint8_t { Val = 0,
                                     Ptr,
                                     Struct };

/// The parameters to a `anydsl_launch_kernel()` call.
typedef struct {
    void** data;
    const uint32_t* sizes;
    const uint32_t* aligns;
    const uint32_t* alloc_sizes;
    const KernelArgType* types;
} LaunchParamsArgs;

AnyDSL_runtime_std_API void anydsl_launch_kernel(
    int32_t mask, const char* file_name, const char* kernel_name,
    const uint32_t* grid, const uint32_t* block,
    void** arg_data,
    const uint32_t* arg_sizes,
    const uint32_t* arg_aligns,
    const uint32_t* arg_alloc_sizes,
    const uint8_t* arg_types,
    uint32_t num_args)
{
    AnyDSLDevice device = unwrapDevice(anydsl_std_get_device(mask & 0x0F, mask >> 4));

    LaunchParamsArgs args = {
        arg_data,
        arg_sizes,
        arg_aligns,
        arg_alloc_sizes,
        reinterpret_cast<const KernelArgType*>(arg_types)
    };
    AnyDSLInternal::unused(num_args);

    AnyDSLLaunchKernelInfo info = {
        AnyDSL_STRUCTURE_TYPE_DEVICE_LAUNCH_KERNEL_INFO,
        nullptr,
        file_name,
        kernel_name,
        { grid[0], grid[1], grid[2] },
        { block[0], block[1], block[2] },
        args.data
    };

    anydslLaunchKernel(device, &info);
}

AnyDSL_runtime_std_API uint64_t anydsl_std_create_buffer(uint64_t deviceHandle, int64_t size, int32_t flags)
{
    if (size < 0) {
        anydslLogReportMessage(AnyDSL_LOG_REPORT_LEVEL_ERROR_BIT, "Trying to allocate with negative size");
        return 0; // TODO
    }

    AnyDSLDevice device = unwrapDevice(deviceHandle);

    AnyDSLCreateBufferFlags info_flags = 0;
    if (AnyDSL_CHECK_BIT(flags, 0x1))
        info_flags |= AnyDSL_CREATE_BUFFER_HOST_BIT;

    if (AnyDSL_CHECK_BIT(flags, 0x2))
        info_flags |= AnyDSL_CREATE_BUFFER_MANAGED_BIT;

    AnyDSLCreateBufferInfo info = {
        AnyDSL_STRUCTURE_TYPE_CREATE_BUFFER_INFO,
        nullptr,
        (AnyDSLDeviceSize)size,
        info_flags
    };

    AnyDSLBuffer buffer;
    anydslCreateBuffer(device, &info, &buffer);

    return (uint64_t)buffer;
}

AnyDSL_runtime_std_API void anydsl_std_destroy_buffer(uint64_t bufferHandle)
{
    AnyDSLBuffer buffer = unwrapBuffer(bufferHandle);
    anydslDestroyBuffer(buffer);
}

AnyDSL_runtime_std_API const char* anydsl_std_get_device_ptr(uint64_t bufferHandle)
{
    AnyDSLBuffer buffer = unwrapBuffer(bufferHandle);

    AnyDSLGetBufferPointerInfo info = {
        AnyDSL_STRUCTURE_TYPE_GET_BUFFER_POINTER_INFO,
        nullptr,
        (AnyDSLDevicePointer)0 // Will be set by the function
    };
    anydslGetBufferPointer(buffer, &info);

    return reinterpret_cast<const char*>(info.pointer);
}

AnyDSL_runtime_std_API void anydsl_std_copy_buffer(uint64_t bufferSrcHandle, int64_t offsetSrc,
                                                   uint64_t bufferDstHandle, int64_t offsetDst,
                                                   int64_t size)
{
    if (size < 0) {
        anydslLogReportMessage(AnyDSL_LOG_REPORT_LEVEL_ERROR_BIT, "Trying to copy buffer with negative size");
        return;
    }

    if (offsetSrc < 0 || offsetDst < 0) {
        anydslLogReportMessage(AnyDSL_LOG_REPORT_LEVEL_ERROR_BIT, "Trying to use copy buffer with negative offsets");
        return;
    }

    AnyDSLBuffer bufferSrc = unwrapBuffer(bufferSrcHandle);
    AnyDSLBuffer bufferDst = unwrapBuffer(bufferDstHandle);

    AnyDSLBufferCopy region = {
        (AnyDSLDeviceSize)offsetSrc,
        (AnyDSLDeviceSize)offsetDst,
        (AnyDSLDeviceSize)size
    };
    anydslCopyBuffer(bufferSrc, bufferDst, 1, &region);
}

AnyDSL_runtime_std_API void anydsl_std_fill_buffer(uint32_t value,
                                                   uint64_t bufferDstHandle, int64_t offsetDst,
                                                   int64_t size)
{
    if (size < 0) {
        anydslLogReportMessage(AnyDSL_LOG_REPORT_LEVEL_ERROR_BIT, "Trying to fill buffer with negative size");
        return;
    }

    if (offsetDst < 0) {
        anydslLogReportMessage(AnyDSL_LOG_REPORT_LEVEL_ERROR_BIT, "Trying to use fill buffer with negative offsets");
        return;
    }

    AnyDSLBuffer bufferDst = unwrapBuffer(bufferDstHandle);
    anydslFillBuffer(bufferDst, (AnyDSLDeviceSize)offsetDst, (AnyDSLDeviceSize)size, value);
}

AnyDSL_runtime_std_API void anydsl_std_update_buffer(const char* ptr,
                                                     uint64_t bufferDstHandle, int64_t offsetDst,
                                                     int64_t size)
{
    if (size < 0) {
        anydslLogReportMessage(AnyDSL_LOG_REPORT_LEVEL_ERROR_BIT, "Trying to fill buffer with negative size");
        return;
    }

    if (offsetDst < 0) {
        anydslLogReportMessage(AnyDSL_LOG_REPORT_LEVEL_ERROR_BIT, "Trying to use fill buffer with negative offsets");
        return;
    }

    AnyDSLBuffer bufferDst = unwrapBuffer(bufferDstHandle);
    anydslUpdateBuffer(bufferDst, (AnyDSLDeviceSize)offsetDst, (AnyDSLDeviceSize)size, ptr);
}

} // extern "C"
