#pragma once

#include "anydsl_runtime.h"

namespace AnyDSLInternal {
class Device;
/// @brief Device specific memory buffer
class Buffer {
public:
    Buffer(Device* device)
        : mDevice(device)
    {
    }

    virtual ~Buffer() {}

    virtual AnyDSLResult create(const AnyDSLCreateBufferInfo* pInfo)                                       = 0;
    virtual AnyDSLResult destroy()                                                                         = 0;
    virtual AnyDSLResult get_pointer(AnyDSLGetBufferPointerInfo* pInfo)                                    = 0;
    virtual AnyDSLResult copy_to(Buffer* dst, uint32_t count, const AnyDSLBufferCopy* pRegions)            = 0;
    virtual AnyDSLResult fill(AnyDSLDeviceSize offset, AnyDSLDeviceSize size, uint32_t data)               = 0;
    virtual AnyDSLResult copy_from_host(AnyDSLDeviceSize offset, AnyDSLDeviceSize size, const void* pData) = 0;
    virtual AnyDSLResult copy_to_host(AnyDSLDeviceSize offset, AnyDSLDeviceSize size, void* pData)         = 0;

    inline Device* device() { return mDevice; }

protected:
    Device* mDevice;
};
} // namespace AnyDSLInternal
