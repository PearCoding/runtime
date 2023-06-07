#pragma once

#include "cpu_device.h"
#include "platform.h"

namespace AnyDSLInternal {

/// CPU platform, allocation is guaranteed to be aligned to page size: 4096 bytes.
class CpuPlatform : public Platform {
public:
    CpuPlatform(Runtime* runtime);

    AnyDSLResult init() override;
    std::tuple<AnyDSLResult, Device*> get_device(const AnyDSLGetDeviceRequest* pInfo) override;
    void append_device_infos(AnyDSLDeviceInfo* pInfo, size_t maxCount) override;

    std::string name() const override { return "CPU"; }
    size_t dev_count() const override { return 1; }
    AnyDSLDeviceType type() const override { return AnyDSL_DEVICE_HOST; }

    inline CpuDevice* host() { return &mHost; }

protected:
    CpuDevice mHost;
};

} // namespace AnyDSLInternal