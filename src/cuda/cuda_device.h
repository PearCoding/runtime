#pragma once

#include "cuda_inc.h"
#include "device.h"

#include <mutex>
#include <string>
#include <unordered_map>

namespace AnyDSLInternal {
class CudaDevice : public Device {
public:
    CudaDevice(int devId, Platform* platform)
        : Device(platform)
        , mId(devId)
        , mDumpCubin(false)
    {
    }

    // Careful, only added to support emplace_back
    CudaDevice(CudaDevice&& device)
        : Device(device.mPlatform)
        , mId(device.mId)
    {
    }

    virtual ~CudaDevice() {}

    AnyDSLResult init();

    AnyDSLResult get_handle(AnyDSLDeviceHandleInfo* pInfo) override;
    AnyDSLResult get_info(AnyDSLDeviceInfo* pInfo) override;
    AnyDSLResult get_features(AnyDSLDeviceFeatures* pFeatures) override;
    AnyDSLResult set_options(AnyDSLDeviceOptions* pOptions) override;

    AnyDSLResult sync() override;

    std::tuple<AnyDSLResult, Buffer*> create_buffer(const AnyDSLCreateBufferInfo* pInfo) override;
    std::tuple<AnyDSLResult, Event*> create_event(const AnyDSLCreateEventInfo* pInfo) override;

    AnyDSLResult launch_kernel(const AnyDSLLaunchKernelInfo* pInfo) override;

    inline bool isHost() const override { return false; }

    inline int id() const { return mId; }
    inline CUdevice handle() const { return mDevice; }
    inline CUcontext context() const { return mContext; }

    void make_context();
    void drop_context();

private:
    AnyDSLResult load_kernel(const std::string& filename, const std::string& kernelname, CUfunction* func);
    AnyDSLResult compile_nvvm(const std::string& filename, const std::string& program_string, std::string* compiled) const;
    AnyDSLResult create_module(const std::string& filename, const std::string& ptx_string, CUmodule* module) const;

    int mId;
    CUdevice mDevice;
    CUcontext mContext;
    std::string mName;
    int mMajorVersion;
    int mMinorVersion;
    CUjit_target mComputeCapability;

    bool mDumpCubin;

    using FunctionMap = std::unordered_map<std::string, CUfunction>;

    std::mutex mLock;
    std::unordered_map<std::string, CUmodule> mModules;
    std::unordered_map<CUmodule, FunctionMap> mFunctions;
};

struct CudaContextGuard {
    CudaDevice* device;

    inline CudaContextGuard(Device* device)
        : device((CudaDevice*)device)
    {
        this->device->make_context();
    }

    inline ~CudaContextGuard()
    {
        device->drop_context();
    }
};
} // namespace AnyDSLInternal
