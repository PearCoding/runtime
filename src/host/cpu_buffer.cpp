#include "cpu_buffer.h"
#include "device.h"

#include <algorithm>
#include <cstring>

namespace AnyDSLInternal {
AnyDSLResult CpuBuffer::create(const AnyDSLCreateBufferInfo* pInfo)
{
    mMem = new std::byte[pInfo->size];
    if (mMem == nullptr)
        return AnyDSL_OUT_OF_HOST_MEMORY;

    return AnyDSL_SUCCESS;
}

AnyDSLResult CpuBuffer::destroy()
{
    if (mMem != nullptr) {
        delete[] mMem;
        mMem = nullptr;
    }

    return AnyDSL_SUCCESS;
}

AnyDSLResult CpuBuffer::get_pointer(AnyDSLGetBufferPointerInfo* pInfo)
{
    pInfo->pointer = (AnyDSLDevicePointer)mMem;
    return AnyDSL_SUCCESS;
}

AnyDSLResult CpuBuffer::copy_to(Buffer* dst, uint32_t count, const AnyDSLBufferCopy* pRegions)
{
    if (dst->device() == device() || dst->device() == (Device*)AnyDSL_HOST || dst->device()->isHost()) {
        // Only host
        for (uint32_t i = 0; i < count; ++i) {
            void* src_mem = mMem + pRegions[i].offsetSrc;
            void* dst_mem = ((CpuBuffer*)dst)->mMem + pRegions[i].offsetDst;
            std::memcpy(dst_mem, src_mem, pRegions[i].size);
        }
    } else {
        return AnyDSL_NOT_SUPPORTED;
    }

    return AnyDSL_SUCCESS;
}

AnyDSLResult CpuBuffer::fill(AnyDSLDeviceSize offset, AnyDSLDeviceSize size, uint32_t data)
{
    std::fill_n((uint32_t*)(mMem + offset), size, data);
    return AnyDSL_SUCCESS;
}

AnyDSLResult CpuBuffer::update(AnyDSLDeviceSize offset, AnyDSLDeviceSize size, const void* pData)
{
    std::memcpy(mMem + offset, pData, size);
    return AnyDSL_SUCCESS;
}

} // namespace AnyDSLInternal