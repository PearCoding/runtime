#include "cpu_platform.h"
#include "utils.h"

namespace AnyDSLInternal {
CpuPlatform::CpuPlatform(Runtime* runtime)
    : Platform(runtime)
    , mHost(this)
{
}

AnyDSLResult CpuPlatform::init()
{
    return mHost.init();
}

std::tuple<AnyDSLResult, Device*> CpuPlatform::get_device(const AnyDSLGetDeviceRequest* pRequest)
{
    if (pRequest == nullptr || pRequest->sType != AnyDSL_STRUCTURE_TYPE_GET_DEVICE_REQUEST)
        return { HANDLE_ERROR(AnyDSL_INVALID_VALUE), nullptr };

    return { AnyDSL_SUCCESS, &mHost };
}

void CpuPlatform::append_device_infos(AnyDSLDeviceInfo* pInfo, size_t maxCount)
{
    if (maxCount > 0)
        mHost.get_info(pInfo);
}

} // namespace AnyDSLInternal
