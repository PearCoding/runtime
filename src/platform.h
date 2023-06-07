#pragma once

#include "anydsl_runtime.h"

#include <cstdint>
#include <string>
#include <tuple>

namespace AnyDSLInternal {
class Runtime;
class Device;
/// A runtime platform.
class Platform {
public:
    Platform(Runtime* runtime)
        : mRuntime(runtime)
    {
    }

    virtual ~Platform() {}

    virtual AnyDSLResult init()                                                            = 0;
    virtual std::tuple<AnyDSLResult, Device*> get_device(const AnyDSLGetDeviceRequest* pInfo) = 0;
    virtual void append_device_infos(AnyDSLDeviceInfo* pInfo, size_t maxCount)             = 0;

    /// Returns the platform name.
    virtual std::string name() const = 0;
    /// Returns the number of devices in this platform.
    virtual size_t dev_count() const = 0;

    virtual AnyDSLDeviceType type() const = 0;

protected:
    Runtime* mRuntime;
};
} // namespace AnyDSLInternal
