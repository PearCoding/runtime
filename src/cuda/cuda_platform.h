#pragma once

#include "cuda_device.h"
#include "platform.h"
#include "runtime.h"

#include <atomic>
#include <forward_list>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace AnyDSLInternal {
/// CUDA platform. Has the same number of devices as that of the CUDA implementation.
class CudaPlatform : public Platform {
public:
    CudaPlatform(Runtime* runtime);
    ~CudaPlatform();

    AnyDSLResult init() override;
    std::tuple<AnyDSLResult, Device*> get_device(const AnyDSLGetDeviceRequest* pInfo) override;
    void append_device_infos(AnyDSLDeviceInfo* pInfo, size_t maxCount) override;

    std::string name() const override { return "CUDA"; }
    size_t dev_count() const override { return mDevices.size(); }
    AnyDSLDeviceType type() const override { return AnyDSL_DEVICE_CUDA; }

protected:
    std::vector<CudaDevice> mDevices;
};
} // namespace AnyDSLInternal