#pragma once

#include "buffer.h"
#include <cstddef>

namespace AnyDSLInternal {
class CpuBuffer : public Buffer {
public:
    CpuBuffer(Device* device)
        : Buffer(device)
        , mMem(0)
    {
    }

    virtual ~CpuBuffer() {}

    AnyDSLResult create(const AnyDSLCreateBufferInfo* pInfo) override;
    AnyDSLResult destroy() override;
    AnyDSLResult get_pointer(AnyDSLGetBufferPointerInfo* pInfo) override;
    AnyDSLResult copy_to(Buffer* dst, uint32_t count, const AnyDSLBufferCopy* pRegions) override;
    AnyDSLResult fill(AnyDSLDeviceSize offset, AnyDSLDeviceSize size, uint32_t data) override;
    AnyDSLResult update(AnyDSLDeviceSize offset, AnyDSLDeviceSize size, const void* pData) override;

    inline std::byte* pointer() const { return mMem; }

private:
    std::byte* mMem;
};
} // namespace AnyDSLInternal
