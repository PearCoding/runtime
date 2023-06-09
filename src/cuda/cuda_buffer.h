#pragma once

#include "buffer.h"
#include "cuda_device.h"
#include "cuda_inc.h"

namespace AnyDSLInternal {
class CudaBuffer : public Buffer {
public:
    CudaBuffer(CudaDevice* device)
        : Buffer(device)
        , mMem(0)
    {
    }

    virtual ~CudaBuffer() {}

    AnyDSLResult create(const AnyDSLCreateBufferInfo* pInfo) override;
    AnyDSLResult destroy() override;
    AnyDSLResult get_pointer(AnyDSLGetBufferPointerInfo* pInfo) override;
    AnyDSLResult copy_to(Buffer* dst, uint32_t count, const AnyDSLBufferCopy* pRegions) override;
    AnyDSLResult fill(AnyDSLDeviceSize offset, AnyDSLDeviceSize size, uint32_t data) override;
    AnyDSLResult copy_from_host(AnyDSLDeviceSize offset, AnyDSLDeviceSize size, const void* pData) override;
    AnyDSLResult copy_to_host(AnyDSLDeviceSize offset, AnyDSLDeviceSize size, void* pData) override;

private:
    uintptr_t mMem;
    uint32_t mFlags;
};
} // namespace AnyDSLInternal
