#include "cuda_buffer.h"
#include "host/cpu_buffer.h"

namespace AnyDSLInternal {
AnyDSLResult CudaBuffer::create(const AnyDSLCreateBufferInfo* pInfo)
{
    mFlags = pInfo->flags;

    CudaContextGuard ctx(mDevice);

    if (AnyDSL_CHECK_BIT(mFlags, AnyDSL_CREATE_BUFFER_MANAGED_BIT)) {
        CUdeviceptr mem;
        CUresult err = cuMemAllocManaged(&mem, pInfo->size, CU_MEM_ATTACH_GLOBAL);
        CHECK_CUDA_RET(err, "cuMemAllocManaged()");

        mMem = (uintptr_t)mem;
    } else if (AnyDSL_CHECK_BIT(mFlags, AnyDSL_CREATE_BUFFER_HOST_BIT)) {
        void* mem;
        CUresult err = cuMemHostAlloc(&mem, pInfo->size, CU_MEMHOSTALLOC_DEVICEMAP);
        CHECK_CUDA_RET(err, "cuMemHostAlloc()");

        mMem = (uintptr_t)mem;
    } else {
        CUdeviceptr mem;
        CUresult err = cuMemAlloc(&mem, pInfo->size);
        CHECK_CUDA_RET(err, "cuMemAlloc()");

        mMem = (uintptr_t)mem;
    }

    return AnyDSL_SUCCESS;
}

AnyDSLResult CudaBuffer::destroy()
{
    CudaContextGuard ctx(mDevice);

    if (AnyDSL_CHECK_BIT(mFlags, AnyDSL_CREATE_BUFFER_HOST_BIT)) {
        CUresult err = cuMemFreeHost((void*)mMem);
        CHECK_CUDA_RET(err, "cuMemFreeHost()");
    } else {
        CUresult err = cuMemFree((CUdeviceptr)mMem);
        CHECK_CUDA_RET(err, "cuMemFree()");
    }

    return AnyDSL_SUCCESS;
}

AnyDSLResult CudaBuffer::get_pointer(AnyDSLGetBufferPointerInfo* pInfo)
{
    if (AnyDSL_CHECK_BIT(mFlags, AnyDSL_CREATE_BUFFER_HOST_BIT)) {
        CudaContextGuard ctx(mDevice);

        CUdeviceptr mem;
        CUresult err = cuMemHostGetDevicePointer(&mem, (void*)mMem, 0);
        CHECK_CUDA_RET(err, "cuMemHostGetDevicePointer()");

        pInfo->pointer = mem;
    } else {
        pInfo->pointer = mMem;
    }

    return AnyDSL_SUCCESS;
}

AnyDSLResult CudaBuffer::copy_to(Buffer* dst, uint32_t count, const AnyDSLBufferCopy* pRegions)
{
    // TODO: Make use of asynchronous stuff if possible

    if (dst->device() == device()) {
        // Same device
        CudaContextGuard ctx(mDevice);
        for (uint32_t i = 0; i < count; ++i) {
            CUdeviceptr src_mem = mMem + pRegions[i].offsetSrc;
            CUdeviceptr dst_mem = ((CudaBuffer*)dst)->mMem + pRegions[i].offsetDst;
            CUresult err        = cuMemcpyDtoD(dst_mem, src_mem, pRegions[i].size);
            CHECK_CUDA_RET(err, "cuMemcpyDtoD()");
        }
    } else if (dst->device()->isHost()) {
        // Device to host
        CudaContextGuard ctx(mDevice);
        for (uint32_t i = 0; i < count; ++i) {
            CUdeviceptr src_mem = mMem + pRegions[i].offsetSrc;
            void* dst_mem       = ((CpuBuffer*)dst)->pointer() + pRegions[i].offsetDst;
            CUresult err        = cuMemcpyDtoH(dst_mem, src_mem, pRegions[i].size);
            CHECK_CUDA_RET(err, "cuMemcpyDtoH()");
        }
    } else if (dst->device()->platform() == device()->platform()) {
        // Same platform
        for (uint32_t i = 0; i < count; ++i) {
            CUdeviceptr src_mem = mMem + pRegions[i].offsetSrc;
            CUdeviceptr dst_mem = ((CudaBuffer*)dst)->mMem + pRegions[i].offsetDst;
            CUresult err        = cuMemcpyPeer(dst_mem, ((CudaDevice*)dst->device())->context(), src_mem, ((CudaDevice*)mDevice)->context(), pRegions[i].size);
            CHECK_CUDA_RET(err, "cuMemcpyPeer()");
        }
    } else {
        return AnyDSL_NOT_SUPPORTED;
    }

    return AnyDSL_SUCCESS;
}

AnyDSLResult CudaBuffer::fill(AnyDSLDeviceSize offset, AnyDSLDeviceSize count, uint32_t data)
{
    CudaContextGuard ctx(mDevice);

    CUresult err = cuMemsetD32(mMem + offset, data, count);
    CHECK_CUDA_RET(err, "cuMemsetD32()");

    return AnyDSL_SUCCESS;
}

AnyDSLResult CudaBuffer::copy_from_host(AnyDSLDeviceSize offset, AnyDSLDeviceSize size, const void* pData)
{
    CudaContextGuard ctx(mDevice);

    CUdeviceptr mem = mMem + offset;
    CUresult err    = cuMemcpyHtoD(mem, pData, size);
    CHECK_CUDA_RET(err, "cuMemcpyHtoD()");

    return AnyDSL_SUCCESS;
}

AnyDSLResult CudaBuffer::copy_to_host(AnyDSLDeviceSize offset, AnyDSLDeviceSize size, void* pData)
{
    CudaContextGuard ctx(mDevice);

    CUdeviceptr mem = mMem + offset;
    CUresult err    = cuMemcpyDtoH(pData, mem, size);
    CHECK_CUDA_RET(err, "cuMemcpyDtoH()");

    return AnyDSL_SUCCESS;
}
} // namespace AnyDSLInternal