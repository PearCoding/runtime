#pragma once

#include "anydsl_runtime.h"

namespace AnyDSLInternal {
class Device;
/// @brief Device specific event. The main usage of events is synchronization and profiling.
class Event {
public:
    Event(Device* device)
        : mDevice(device)
    {
    }

    virtual ~Event() {}

    virtual AnyDSLResult create(const AnyDSLCreateEventInfo* pInfo)       = 0;
    virtual AnyDSLResult destroy()                                        = 0;
    virtual AnyDSLResult record()                                         = 0;
    virtual AnyDSLResult query(Event* event, AnyDSLQueryEventInfo* pInfo) = 0;
    virtual AnyDSLResult sync()                                           = 0;

    inline Device* device() { return mDevice; }

protected:
    Device* mDevice;
};
} // namespace AnyDSLInternal
