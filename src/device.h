#pragma once

#include "anydsl_runtime.h"

#include <tuple>

namespace AnyDSLInternal {
class Platform;
class Buffer;
class Event;

/// @brief A platform specific device
class Device {
public:
    Device(Platform* platform)
        : mPlatform(platform)
    {
    }

    virtual ~Device() {}

    virtual AnyDSLResult get_handle(AnyDSLDeviceHandleInfo* pInfo)     = 0;
    virtual AnyDSLResult get_info(AnyDSLDeviceInfo* pInfo)             = 0;
    virtual AnyDSLResult get_features(AnyDSLDeviceFeatures* pFeatures) = 0;
    virtual AnyDSLResult set_options(AnyDSLDeviceOptions* pOptions)    = 0;

    virtual AnyDSLResult sync() = 0;

    virtual std::tuple<AnyDSLResult, Buffer*> create_buffer(const AnyDSLCreateBufferInfo* pInfo) = 0;
    virtual std::tuple<AnyDSLResult, Event*> create_event(const AnyDSLCreateEventInfo* pInfo)    = 0;

    virtual AnyDSLResult launch_kernel(const AnyDSLLaunchKernelInfo* pInfo) = 0;

    virtual bool isHost() const = 0;

    inline Platform* platform() { return mPlatform; }

protected:
    Platform* mPlatform;
};
} // namespace AnyDSLInternal
