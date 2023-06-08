#include "cuda_event.h"
#include "log.h"

namespace AnyDSLInternal {
AnyDSLResult CudaEvent::create(const AnyDSLCreateEventInfo* pInfo)
{
    unused(pInfo);

    CUresult err = cuEventCreate(&mEvent, CU_EVENT_DEFAULT);
    CHECK_CUDA_RET(err, "cuEventCreate()");

    return AnyDSL_SUCCESS;
}

AnyDSLResult CudaEvent::destroy()
{
    CUresult err = cuEventDestroy(mEvent);
    CHECK_CUDA_RET(err, "cuEventDestroy()");

    return AnyDSL_SUCCESS;
}

AnyDSLResult CudaEvent::record()
{
    CUresult err = cuEventRecord(mEvent, 0);
    CHECK_CUDA_RET(err, "cuEventRecord()");

    return AnyDSL_SUCCESS;
}

AnyDSLResult CudaEvent::query(Event* event, AnyDSLQueryEventInfo* pInfo)
{
    if (pInfo != nullptr) {
        CUresult err = cuEventElapsedTime(&pInfo->elapsedTimeMS, mEvent, ((CudaEvent*)event)->mEvent);
        if (err == CUDA_ERROR_NOT_READY)
            return AnyDSL_NOT_READY;
        CHECK_CUDA_RET(err, "cuEventElapsedTime()");
    } else {
        CUresult err = cuEventQuery(mEvent);
        if (err == CUDA_ERROR_NOT_READY)
            return AnyDSL_NOT_READY;
        CHECK_CUDA_RET(err, "cuEventQuery()");

        if (event != nullptr) {
            err = cuEventQuery(((CudaEvent*)event)->mEvent);
            if (err == CUDA_ERROR_NOT_READY)
                return AnyDSL_NOT_READY;
            CHECK_CUDA_RET(err, "cuEventQuery()");
        }
    }

    return AnyDSL_SUCCESS;
}

AnyDSLResult CudaEvent::sync()
{
    CUresult err = cuEventSynchronize(mEvent);
    CHECK_CUDA_RET(err, "cuEventSynchronize()");

    return AnyDSL_SUCCESS;
}
} // namespace AnyDSLInternal