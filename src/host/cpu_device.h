#pragma once

#include "device.h"

#include <string>

namespace AnyDSLInternal {
class CpuDevice : public Device {
public:
    CpuDevice(Platform* platform)
        : Device(platform)
    {
    }

    ~CpuDevice() {}

    AnyDSLResult init();

    AnyDSLResult get_handle(AnyDSLDeviceHandleInfo* pInfo) override;
    AnyDSLResult get_info(AnyDSLDeviceInfo* pInfo) override;
    AnyDSLResult get_features(AnyDSLDeviceFeatures* pFeatures) override;

    AnyDSLResult sync() override;

    std::tuple<AnyDSLResult, Buffer*> create_buffer(const AnyDSLCreateBufferInfo* pInfo) override;
    std::tuple<AnyDSLResult, Event*> create_event(const AnyDSLCreateEventInfo* pInfo) override;

    AnyDSLResult launch_kernel(const AnyDSLLaunchKernelInfo* pInfo) override;

    inline bool isHost() const override { return true; }

private:
    std::string mName;
};
} // namespace AnyDSLInternal
