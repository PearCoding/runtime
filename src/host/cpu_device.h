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
    AnyDSLResult set_options(AnyDSLDeviceOptions* pOptions) override;

    AnyDSLResult sync() override;

    std::tuple<AnyDSLResult, Buffer*> create_buffer(const AnyDSLCreateBufferInfo* pInfo) override;
    std::tuple<AnyDSLResult, Event*> create_event(const AnyDSLCreateEventInfo* pInfo) override;

    std::tuple<AnyDSLResult, void*> allocate_memory(size_t size) override;
    AnyDSLResult release_memory(void* ptr) override;

    AnyDSLResult launch_kernel(const AnyDSLLaunchKernelInfo* pInfo) override;

    inline bool isHost() const override { return true; }

private:
    std::string mName;
};
} // namespace AnyDSLInternal
