#pragma once

#include "event.h"

namespace AnyDSLInternal {
class CudaEvent : public Event {
public:
    CudaEvent(Device* device)
        : Event(device)
    {
    }

    virtual ~CudaEvent() {}

    virtual AnyDSLResult query(AnyDSLQueryEventInfo* pInfo) = 0;
    virtual AnyDSLResult sync()                             = 0;
};
} // namespace AnyDSLInternal
