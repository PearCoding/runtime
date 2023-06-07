#include "cuda_platform.h"
#include "cuda_inc.h"
#include "log.h"
#include "runtime.h"
#include "utils.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>

using namespace std::literals;

namespace AnyDSLInternal {
CudaPlatform::CudaPlatform(Runtime* runtime)
    : Platform(runtime)
{
}

AnyDSLResult CudaPlatform::init()
{
    debug("Initializing CUDA platform");

    int device_count = 0, driver_version = 0, nvvm_major = 0, nvvm_minor = 0;

#ifndef _WIN32
    // TODO: Should this not be a user parameter??
    setenv("CUDA_CACHE_DISABLE", "1", 1);
#endif

    CUresult err = cuInit(0);
    CHECK_CUDA_RET(err, "cuInit()");

    err = cuDeviceGetCount(&device_count);
    CHECK_CUDA_RET(err, "cuDeviceGetCount()");

    err = cuDriverGetVersion(&driver_version);
    CHECK_CUDA_RET(err, "cuDriverGetVersion()");

    nvvmResult err_nvvm = nvvmVersion(&nvvm_major, &nvvm_minor);
    CHECK_NVVM_RET(err_nvvm, "nvvmVersion()");

    debug("CUDA Driver Version %d.%d", driver_version / 1000, (driver_version % 100) / 10);
    debug("NVVM Version %d.%d", nvvm_major, nvvm_minor);

    // initialize devices
    for (int i = 0; i < device_count; ++i) {
        AnyDSLResult result = mDevices.emplace_back(i, this).init();
        if (result != AnyDSL_SUCCESS)
            return result;
    }

    return AnyDSL_SUCCESS;
}

CudaPlatform::~CudaPlatform()
{
}

std::tuple<AnyDSLResult, Device*> CudaPlatform::get_device(const AnyDSLGetDeviceRequest* pRequest)
{
    if (pRequest == nullptr || pRequest->sType != AnyDSL_STRUCTURE_TYPE_GET_DEVICE_REQUEST)
        return { AnyDSL_INVALID_VALUE, nullptr };

    if (pRequest->deviceNumber >= (uint32_t)mDevices.size())
        return { AnyDSL_INVALID_VALUE, nullptr };

    return { AnyDSL_SUCCESS, (Device*)&mDevices.at(pRequest->deviceNumber) };
}

void CudaPlatform::append_device_infos(AnyDSLDeviceInfo* pInfo, size_t maxCount)
{
    for (size_t i = 0; i < mDevices.size() && i < maxCount; ++i) {
        mDevices[i].get_info(pInfo);
        pInfo++;
    }
}

} // namespace AnyDSLInternal