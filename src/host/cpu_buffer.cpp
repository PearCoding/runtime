#include "cpu_buffer.h"
#include "device.h"
#include "runtime.h"

#include <algorithm>
#include <cstring>

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

namespace AnyDSLInternal {
AnyDSLResult CpuBuffer::create(const AnyDSLCreateBufferInfo* pInfo)
{
    if (AnyDSL_CHECK_BIT(pInfo->flags, AnyDSL_CREATE_BUFFER_MANAGED_BIT)) {
        mMem = (std::byte*)Runtime::instance().aligned_malloc(pInfo->size, PAGE_SIZE);
    } else if (AnyDSL_CHECK_BIT(pInfo->flags, AnyDSL_CREATE_BUFFER_HOST_BIT)) {
        mMem = (std::byte*)Runtime::instance().aligned_malloc(pInfo->size, PAGE_SIZE);
    } else {
        mMem = (std::byte*)Runtime::instance().aligned_malloc(pInfo->size, 32);
    }

    if (mMem == nullptr)
        return AnyDSL_OUT_OF_HOST_MEMORY;

    return AnyDSL_SUCCESS;
}

AnyDSLResult CpuBuffer::destroy()
{
    if (mMem != nullptr) {
        Runtime::instance().aligned_free(mMem);
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

AnyDSLResult CpuBuffer::fill(AnyDSLDeviceSize offset, AnyDSLDeviceSize count, uint32_t data)
{
    std::fill_n((uint32_t*)(mMem + offset), count, data);
    return AnyDSL_SUCCESS;
}

AnyDSLResult CpuBuffer::copy_from_host(AnyDSLDeviceSize offset, AnyDSLDeviceSize size, const void* pData)
{
    std::memcpy(mMem + offset, pData, size);
    return AnyDSL_SUCCESS;
}

AnyDSLResult CpuBuffer::copy_to_host(AnyDSLDeviceSize offset, AnyDSLDeviceSize size, void* pData)
{
    std::memcpy(pData, mMem + offset, size);
    return AnyDSL_SUCCESS;
}

} // namespace AnyDSLInternal