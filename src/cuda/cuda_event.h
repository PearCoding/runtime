#pragma once

#include "cuda_device.h"
#include "cuda_inc.h"
#include "event.h"

namespace AnyDSLInternal {
class CudaEvent : public Event {
public:
    CudaEvent(Device* device)
        : Event(device)
    {
    }

    virtual ~CudaEvent() {}

    AnyDSLResult create(const AnyDSLCreateEventInfo* pInfo) override;
    AnyDSLResult destroy() override;
    AnyDSLResult record() override;
    AnyDSLResult query(Event* event, AnyDSLQueryEventInfo* pInfo) override;
    AnyDSLResult sync() override;

private:
    CUevent mEvent;
};
} // namespace AnyDSLInternal
