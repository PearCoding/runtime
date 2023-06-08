#include "cpu_event.h"
#include "log.h"

namespace AnyDSLInternal {
// TODO: Maybe make this a bit more lightweight?

AnyDSLResult CpuEvent::create(const AnyDSLCreateEventInfo* pInfo)
{
    unused(pInfo);
    return AnyDSL_SUCCESS;
}

AnyDSLResult CpuEvent::destroy()
{
    return AnyDSL_SUCCESS;
}

AnyDSLResult CpuEvent::record()
{
    std::unique_lock lk(mMutex);
    mRecorded      = true;
    mPointOfRecord = std::chrono::high_resolution_clock::now();
    lk.unlock();

    mCV.notify_all();

    return AnyDSL_SUCCESS;
}

AnyDSLResult CpuEvent::query(Event* event, AnyDSLQueryEventInfo* pInfo)
{
    std::lock_guard _guard(mMutex);

    if (pInfo != nullptr) {
        if (!mRecorded || !((CpuEvent*)event)->mRecorded)
            return AnyDSL_NOT_READY;

        pInfo->elapsedTimeMS = (float)std::chrono::duration_cast<std::chrono::microseconds>(((CpuEvent*)event)->mPointOfRecord - mPointOfRecord).count() / 1000.0f;
    } else {
        if (!mRecorded)
            return AnyDSL_NOT_READY;

        if (event != nullptr) {
            if (!((CpuEvent*)event)->mRecorded)
                return AnyDSL_NOT_READY;
        }
    }

    return AnyDSL_SUCCESS;
}

AnyDSLResult CpuEvent::sync()
{
    std::unique_lock lk(mMutex);
    mCV.wait(lk, [this]() { return this->mRecorded; });
    return AnyDSL_SUCCESS;
}
} // namespace AnyDSLInternal