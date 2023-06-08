#pragma once

#include "event.h"

#include <chrono>
#include <condition_variable>
#include <mutex>

namespace AnyDSLInternal {
class CpuEvent : public Event {
public:
    CpuEvent(Device* device)
        : Event(device)
    {
    }

    virtual ~CpuEvent() {}

    AnyDSLResult create(const AnyDSLCreateEventInfo* pInfo) override;
    AnyDSLResult destroy() override;
    AnyDSLResult record() override;
    AnyDSLResult query(Event* event, AnyDSLQueryEventInfo* pInfo) override;
    AnyDSLResult sync() override;

private:
    std::mutex mMutex;
    std::condition_variable mCV;
    bool mRecorded;
    std::chrono::high_resolution_clock::time_point mPointOfRecord;
};
} // namespace AnyDSLInternal
