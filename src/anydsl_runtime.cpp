#include "anydsl_runtime.h"
#include "anydsl_runtime_internal_config.h"

#include "buffer.h"
#include "device.h"
#include "event.h"
#include "log.h"
#include "utils.h"

#include "jit.h"

#include "host/cpu_platform.h"
#include "platform.h"
#include "runtime.h"

using namespace AnyDSLInternal;

static inline bool checkHandle(void* ptr) { return ptr != nullptr; }

static inline Device* unwrapDeviceHandle(AnyDSLDevice device) { return (Device*)device; }
static inline Buffer* unwrapBufferHandle(AnyDSLBuffer buffer) { return (Buffer*)buffer; }
static inline Event* unwrapEventHandle(AnyDSLEvent event) { return (Event*)event; }

// ----------------------------------------- Runtime
AnyDSLResult anydslGetVersion(AnyDSLVersion* pVersion)
{
    ANYDSL_CHECK_RET_PTR(pVersion);

    // TODO
    pVersion->major = 2;
    pVersion->minor = 0;
    pVersion->patch = 0;

    return AnyDSL_SUCCESS;
}

AnyDSLResult anydslGetFeatures(AnyDSLFeatures* pFeatures)
{
    ANYDSL_CHECK_RET_PTR(pFeatures);

#ifdef AnyDSL_runtime_HAS_JIT_SUPPORT
    pFeatures->hasJIT = AnyDSL_TRUE;
#else
    pFeatures->hasJIT = AnyDSL_FALSE;
#endif

    pFeatures->supportedLanguages = 0;
#ifdef AnyDSL_runtime_HAS_ARTIC_LANGUAGE
    pFeatures->supportedLanguages |= AnyDSL_COMPILE_LANGUAGE_ARTIC_BIT;
#endif

#ifdef AnyDSL_runtime_HAS_IMPALA_LANGUAGE
    pFeatures->supportedLanguages |= AnyDSL_COMPILE_LANGUAGE_IMPALA_BIT;
#endif

    return AnyDSL_SUCCESS;
}

AnyDSLResult anydslEnumerateDevices(size_t* pCount, AnyDSLDeviceInfo* pInfo)
{
    ANYDSL_CHECK_RET_PTR(pCount);

    if (pInfo == nullptr) {
        // Compute count
        for (size_t i = 0; i < Runtime::instance().platforms().size(); ++i) {
            Platform* platform = Runtime::instance().platforms()[i].get();
            *pCount += (size_t)platform->dev_count();
        }
    } else {
        const size_t maxCount = *pCount;

        for (size_t k = 0; k < maxCount; ++k) {
            ANYDSL_CHECK_RET_TYPE(&pInfo[k], AnyDSL_STRUCTURE_TYPE_DEVICE_INFO);
        }

        size_t c = 0;
        for (size_t i = 0; i < Runtime::instance().platforms().size() && c < maxCount; ++i) {
            Platform* platform = Runtime::instance().platforms()[i].get();
            platform->append_device_infos(pInfo, maxCount - c);
            c += (size_t)platform->dev_count();
            pInfo += platform->dev_count(); // Iterate through array
        }
    }

    return AnyDSL_SUCCESS;
}

// ----------------------------------------- Device
AnyDSLResult anydslGetDevice(const AnyDSLGetDeviceRequest* pRequest, AnyDSLDevice* pDevice)
{
    ANYDSL_CHECK_RET_PTR(pRequest);
    ANYDSL_CHECK_RET_PTR(pDevice);
    ANYDSL_CHECK_RET_TYPE(pRequest, AnyDSL_STRUCTURE_TYPE_GET_DEVICE_REQUEST);

    Platform* platform = Runtime::instance().query_platform(pRequest->deviceType).value_or(nullptr);
    if (platform == nullptr)
        return AnyDSL_INVALID_VALUE;

    const auto pair = platform->get_device(pRequest);
    if (std::get<1>(pair) == nullptr)
        return std::get<0>(pair);

    *pDevice = (AnyDSLDevice)std::get<1>(pair);
    return AnyDSL_SUCCESS;
}

AnyDSLResult anydslGetDeviceHandle(AnyDSLDevice device, AnyDSLDeviceHandleInfo* pHandleInfo)
{
    if (!checkHandle(device))
        return AnyDSL_INVALID_HANDLE;

    return unwrapDeviceHandle(device)->get_handle(pHandleInfo);
}

AnyDSLResult anydslGetDeviceInfo(AnyDSLDevice device, AnyDSLDeviceInfo* pDeviceInfo)
{
    if (!checkHandle(device))
        return AnyDSL_INVALID_HANDLE;

    return unwrapDeviceHandle(device)->get_info(pDeviceInfo);
}

AnyDSLResult anydslGetDeviceFeatures(AnyDSLDevice device, AnyDSLDeviceFeatures* pDeviceFeatures)
{
    if (!checkHandle(device))
        return AnyDSL_INVALID_HANDLE;

    return unwrapDeviceHandle(device)->get_features(pDeviceFeatures);
}

AnyDSLResult anydslSetDeviceOptions(AnyDSLDevice device, AnyDSLDeviceOptions* pDeviceOptions)
{
    if (!checkHandle(device))
        return AnyDSL_INVALID_HANDLE;

    return unwrapDeviceHandle(device)->set_options(pDeviceOptions);
}

AnyDSLResult anydslSynchronizeDevice(AnyDSLDevice device)
{
    if (device == AnyDSL_HOST)
        return AnyDSL_SUCCESS;

    if (!checkHandle(device))
        return AnyDSL_INVALID_HANDLE;

    return unwrapDeviceHandle(device)->sync();
}

AnyDSLResult anydslLaunchKernel(AnyDSLDevice device, const AnyDSLLaunchKernelInfo* pInfo)
{
    if (!checkHandle(device))
        return AnyDSL_INVALID_HANDLE; // Really?

    return unwrapDeviceHandle(device)->launch_kernel(pInfo);
}

// ----------------------------------------- Buffer
AnyDSLResult anydslCreateBuffer(AnyDSLDevice device, const AnyDSLCreateBufferInfo* pInfo, AnyDSLBuffer* pBuffer)
{
    ANYDSL_CHECK_RET_PTR(pBuffer);
    ANYDSL_CHECK_RET_PTR(pInfo);
    ANYDSL_CHECK_RET_TYPE(pInfo, AnyDSL_STRUCTURE_TYPE_CREATE_BUFFER_INFO);

    if (device == AnyDSL_HOST) {
        CpuPlatform* cpu = (CpuPlatform*)Runtime::instance().host();
        const auto pair  = cpu->host()->create_buffer(pInfo);
        if (std::get<1>(pair) == nullptr)
            return std::get<0>(pair);

        *pBuffer = (AnyDSLBuffer)std::get<1>(pair);
        return std::get<0>(pair);
    } else {
        if (!checkHandle(device))
            return AnyDSL_INVALID_HANDLE;

        Device* actualDevice = unwrapDeviceHandle(device);
        const auto pair      = actualDevice->create_buffer(pInfo);
        if (std::get<1>(pair) == nullptr)
            return std::get<0>(pair);

        *pBuffer = (AnyDSLBuffer)std::get<1>(pair);
        return std::get<0>(pair);
    }
}

AnyDSLResult anydslDestroyBuffer(AnyDSLBuffer buffer)
{
    if (!checkHandle(buffer))
        return AnyDSL_INVALID_HANDLE;

    Buffer* actualBuffer = unwrapBufferHandle(buffer);
    AnyDSLResult res     = actualBuffer->destroy();
    delete actualBuffer;
    return res;
}

AnyDSLResult anydslGetBufferPointer(AnyDSLBuffer buffer, AnyDSLGetBufferPointerInfo* pInfo)
{
    if (!checkHandle(buffer))
        return AnyDSL_INVALID_HANDLE;

    return unwrapBufferHandle(buffer)->get_pointer(pInfo);
}

AnyDSLResult anydslCopyBuffer(AnyDSLBuffer bufferSrc, AnyDSLBuffer bufferDst, uint32_t count, const AnyDSLBufferCopy* pRegions)
{
    if (!checkHandle(bufferSrc) || !checkHandle(bufferDst))
        return AnyDSL_INVALID_HANDLE;

    return unwrapBufferHandle(bufferSrc)->copy_to(unwrapBufferHandle(bufferDst), count, pRegions);
}

AnyDSLResult anydslFillBuffer(AnyDSLBuffer bufferDst, AnyDSLDeviceSize offset, AnyDSLDeviceSize size, uint32_t data)
{
    if (!checkHandle(bufferDst))
        return AnyDSL_INVALID_HANDLE;

    return unwrapBufferHandle(bufferDst)->fill(offset, size, data);
}

AnyDSLResult anydslUpdateBuffer(AnyDSLBuffer bufferDst, AnyDSLDeviceSize offset, AnyDSLDeviceSize size, const void* pData)
{
    if (!checkHandle(bufferDst))
        return AnyDSL_INVALID_HANDLE;

    if (pData == nullptr)
        return AnyDSL_INVALID_POINTER;

    return unwrapBufferHandle(bufferDst)->update(offset, size, pData);
}

// ----------------------------------------- Events
AnyDSLResult anydslCreateEvent(AnyDSLDevice device, const AnyDSLCreateEventInfo* pInfo, AnyDSLEvent* pEvent)
{
    // TODO: Host

    if (!checkHandle(device))
        return AnyDSL_INVALID_HANDLE;
    ANYDSL_CHECK_RET_PTR(pEvent);
    ANYDSL_CHECK_RET_PTR(pInfo);
    ANYDSL_CHECK_RET_TYPE(pInfo, AnyDSL_STRUCTURE_TYPE_CREATE_EVENT_INFO);

    Device* actualDevice = unwrapDeviceHandle(device);
    const auto pair      = actualDevice->create_event(pInfo);
    if (std::get<1>(pair) == nullptr)
        return std::get<0>(pair);

    *pEvent = (AnyDSLEvent)std::get<1>(pair);
    return std::get<0>(pair);
}

AnyDSLResult anydslDestroyEvent(AnyDSLEvent event)
{
    if (!checkHandle(event))
        return AnyDSL_INVALID_HANDLE;

    Event* actualEvent = unwrapEventHandle(event);
    AnyDSLResult res   = actualEvent->destroy();
    delete actualEvent;
    return res;
}

AnyDSLResult anydslRecordEvent(AnyDSLEvent event)
{
    if (!checkHandle(event))
        return AnyDSL_INVALID_HANDLE;

    return unwrapEventHandle(event)->record();
}

AnyDSLResult anydslQueryEvent(AnyDSLEvent startEvent, AnyDSLEvent endEvent, AnyDSLQueryEventInfo* pInfo)
{
    if (!checkHandle(startEvent))
        return AnyDSL_INVALID_HANDLE;

    if (pInfo != nullptr)
        ANYDSL_CHECK_RET_TYPE(pInfo, AnyDSL_STRUCTURE_TYPE_QUERY_EVENT_INFO);

    if (pInfo != nullptr && !checkHandle(endEvent))
        return AnyDSL_INVALID_HANDLE;

    Event* ev1 = unwrapEventHandle(startEvent);
    Event* ev2 = unwrapEventHandle(endEvent);

    if (ev2 != nullptr && ev1->device() != ev2->device())
        return AnyDSL_DEVICE_MISSMATCH;

    return ev1->query(ev2, pInfo);
}

AnyDSLResult anydslSynchronizeEvent(AnyDSLEvent event)
{
    if (!checkHandle(event))
        return AnyDSL_INVALID_HANDLE;

    return unwrapEventHandle(event)->sync();
}

// ----------------------------------------- JIT
AnyDSLResult anydslCompileJIT(const char* program, size_t count, AnyDSLJITModule* pModule, const AnyDSLJITCompileOptions* pOptions, AnyDSLJITCompileResult* pResult)
{
    return JIT::compile(program, count, pModule, pOptions, pResult);
}

AnyDSLResult anydslDestroyJITModule(AnyDSLJITModule module)
{
    return JIT::destroyModule(module);
}

AnyDSLResult anydslFreeJITCompileResult(const AnyDSLJITCompileResult* pResult)
{
    return JIT::freeCompileResult(pResult);
}

AnyDSLResult anydslLookupJIT(AnyDSLJITModule module, const char* function, AnyDSLJITLookupInfo* pInfo)
{
    return JIT::lookup(module, function, pInfo);
}

AnyDSLResult anydslLinkJITLibrary(AnyDSLJITModule module, size_t count, const AnyDSLJITLinkInfo* pLinkInfo)
{
    return JIT::link(module, count, pLinkInfo);
}

// ----------------------------------------- Logging
AnyDSL_runtime_API AnyDSLResult anydslCreateLogReportCallback(const AnyDSLLogReportCallbackCreateInfo* pCreateInfo, AnyDSLLogReportCallback* pCallback)
{
    return Log::instance().registerHandler(pCreateInfo, pCallback);
}

AnyDSL_runtime_API AnyDSLResult anydslDestroyLogReportCallback(AnyDSLLogReportCallback callback)
{
    return Log::instance().unregisterHandler(callback);
}

AnyDSL_runtime_API AnyDSLResult anydslLogReportMessage(AnyDSLLogReportLevelFlags flags, const char* pMessage)
{
    return Log::instance().log(flags, pMessage);
}