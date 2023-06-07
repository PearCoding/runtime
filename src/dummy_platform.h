#pragma once

#include "platform.h"
#include "runtime.h"

namespace AnyDSLInternal {
/// Dummy platform
class DummyPlatform : public Platform {
public:
    DummyPlatform(Runtime* runtime, const std::string& name, AnyDSLDeviceType type)
        : Platform(runtime)
        , mName(name)
        , mType(type)
    {
    }

    AnyDSLResult init() override { return AnyDSL_NOT_AVAILABLE; }

    std::tuple<AnyDSLResult, Device*> get_device(const AnyDSLGetDeviceRequest*) override { return { AnyDSL_NOT_AVAILABLE, nullptr }; }
    void append_device_infos(AnyDSLDeviceInfo*, size_t) override {}

    std::string name() const override { return mName; }
    size_t dev_count() const override { return 0; }
    AnyDSLDeviceType type() const override { return mType; }

protected:
    std::string mName;
    AnyDSLDeviceType mType;
};
} // namespace AnyDSLInternal
