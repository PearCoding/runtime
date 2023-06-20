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

#ifdef AnyDSL_RUNTIME_DEBUG
#define TRACE() \
    trace_call("Calling %s [file %s, line %i]", AnyDSL_FUNCTION_NAME, __FILE__, __LINE__)
#else
#define TRACE() \
    trace_call("Calling %s", AnyDSL_FUNCTION_NAME)
#endif

static inline bool checkHandle(void* ptr)
{
    return ptr != nullptr;
}

#define CHECK_HANDLE_RET(ptr) \
    if (!checkHandle(ptr))    \
    return HANDLE_ERROR(AnyDSL_INVALID_HANDLE)

static inline Device* unwrapDeviceHandle(AnyDSLDevice device)
{
    return (Device*)device;
}
static inline Buffer* unwrapBufferHandle(AnyDSLBuffer buffer) { return (Buffer*)buffer; }
static inline Event* unwrapEventHandle(AnyDSLEvent event) { return (Event*)event; }

// ----------------------------------------- Runtime
AnyDSLResult anydslGetVersion(AnyDSLVersion* pVersion)
{
    TRACE();
    CHECK_RET_PTR(pVersion);

    // TODO
    pVersion->major = 2;
    pVersion->minor = 0;
    pVersion->patch = 0;

    return AnyDSL_SUCCESS;
}

AnyDSLResult anydslGetFeatures(AnyDSLFeatures* pFeatures)
{
    TRACE();
    CHECK_RET_PTR(pFeatures);

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
    TRACE();
    CHECK_RET_PTR(pCount);

    if (pInfo == nullptr) {
        // Compute count
        *pCount = 0;
        for (size_t i = 0; i < Runtime::instance().platforms().size(); ++i) {
            Platform* platform = Runtime::instance().platforms()[i].get();
            *pCount += (size_t)platform->dev_count();
        }
    } else {
        const size_t maxCount = *pCount;

        for (size_t k = 0; k < maxCount; ++k) {
            CHECK_RET_TYPE(&pInfo[k], AnyDSL_STRUCTURE_TYPE_DEVICE_INFO);
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
    TRACE();
    CHECK_RET_PTR(pRequest);
    CHECK_RET_PTR(pDevice);
    CHECK_RET_TYPE(pRequest, AnyDSL_STRUCTURE_TYPE_GET_DEVICE_REQUEST);

    Platform* platform = Runtime::instance().query_platform(pRequest->deviceType).value_or(nullptr);
    if (platform == nullptr)
        return HANDLE_ERROR(AnyDSL_INVALID_VALUE);

    const auto pair = platform->get_device(pRequest);
    if (std::get<1>(pair) == nullptr)
        return std::get<0>(pair);

    *pDevice = (AnyDSLDevice)std::get<1>(pair);
    return AnyDSL_SUCCESS;
}

AnyDSLResult anydslGetDeviceHandle(AnyDSLDevice device, AnyDSLDeviceHandleInfo* pHandleInfo)
{
    TRACE();
    CHECK_HANDLE_RET(device);

    return unwrapDeviceHandle(device)->get_handle(pHandleInfo);
}

AnyDSLResult anydslGetDeviceInfo(AnyDSLDevice device, AnyDSLDeviceInfo* pDeviceInfo)
{
    TRACE();
    CHECK_HANDLE_RET(device);

    return unwrapDeviceHandle(device)->get_info(pDeviceInfo);
}

AnyDSLResult anydslGetDeviceFeatures(AnyDSLDevice device, AnyDSLDeviceFeatures* pDeviceFeatures)
{
    TRACE();
    CHECK_HANDLE_RET(device);

    return unwrapDeviceHandle(device)->get_features(pDeviceFeatures);
}

AnyDSLResult anydslSetDeviceOptions(AnyDSLDevice device, const AnyDSLDeviceOptions* pDeviceOptions)
{
    TRACE();
    CHECK_HANDLE_RET(device);

    return unwrapDeviceHandle(device)->set_options(pDeviceOptions);
}

AnyDSLResult anydslSynchronizeDevice(AnyDSLDevice device)
{
    TRACE();
    if (device == AnyDSL_NULL_HANDLE)
        return AnyDSL_SUCCESS;

    CHECK_HANDLE_RET(device);

    return unwrapDeviceHandle(device)->sync();
}

AnyDSLResult anydslLaunchKernel(AnyDSLDevice device, const AnyDSLLaunchKernelInfo* pInfo)
{
    TRACE();
    CHECK_HANDLE_RET(device);

    return unwrapDeviceHandle(device)->launch_kernel(pInfo);
}

// ----------------------------------------- Buffer
AnyDSLResult anydslCreateBuffer(AnyDSLDevice device, const AnyDSLCreateBufferInfo* pInfo, AnyDSLBuffer* pBuffer)
{
    TRACE();
    CHECK_RET_PTR(pBuffer);
    CHECK_RET_PTR(pInfo);
    CHECK_RET_TYPE(pInfo, AnyDSL_STRUCTURE_TYPE_CREATE_BUFFER_INFO);

    if (device == AnyDSL_NULL_HANDLE) {
        CpuPlatform* cpu = (CpuPlatform*)Runtime::instance().host();
        const auto pair  = cpu->host()->create_buffer(pInfo);
        if (std::get<1>(pair) == nullptr)
            return std::get<0>(pair);

        *pBuffer = (AnyDSLBuffer)std::get<1>(pair);
        return std::get<0>(pair);
    } else {
        CHECK_HANDLE_RET(device);

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
    TRACE();
    CHECK_HANDLE_RET(buffer);

    Buffer* actualBuffer = unwrapBufferHandle(buffer);
    AnyDSLResult res     = actualBuffer->destroy();
    delete actualBuffer;
    return res;
}

AnyDSLResult anydslGetBufferPointer(AnyDSLBuffer buffer, AnyDSLGetBufferPointerInfo* pInfo)
{
    TRACE();
    CHECK_HANDLE_RET(buffer);

    return unwrapBufferHandle(buffer)->get_pointer(pInfo);
}

AnyDSLResult anydslCopyBuffer(AnyDSLBuffer bufferSrc, AnyDSLBuffer bufferDst, uint32_t count, const AnyDSLBufferCopy* pRegions)
{
    TRACE();
    CHECK_HANDLE_RET(bufferSrc);
    CHECK_HANDLE_RET(bufferDst);

    return unwrapBufferHandle(bufferSrc)->copy_to(unwrapBufferHandle(bufferDst), count, pRegions);
}

AnyDSLResult anydslFillBuffer(AnyDSLBuffer bufferDst, AnyDSLDeviceSize offset, AnyDSLDeviceSize size, uint32_t data)
{
    TRACE();
    CHECK_HANDLE_RET(bufferDst);

    return unwrapBufferHandle(bufferDst)->fill(offset, size, data);
}

AnyDSLResult anydslCopyBufferFromHost(AnyDSLBuffer bufferDst, AnyDSLDeviceSize offset, AnyDSLDeviceSize size, const void* pData)
{
    TRACE();
    CHECK_HANDLE_RET(bufferDst);

    if (pData == nullptr)
        return HANDLE_ERROR(AnyDSL_INVALID_POINTER);

    return unwrapBufferHandle(bufferDst)->copy_from_host(offset, size, pData);
}

AnyDSLResult anydslCopyBufferToHost(AnyDSLBuffer bufferSrc, AnyDSLDeviceSize offset, AnyDSLDeviceSize size, void* pData)
{
    TRACE();
    CHECK_HANDLE_RET(bufferSrc);

    if (pData == nullptr)
        return HANDLE_ERROR(AnyDSL_INVALID_POINTER);

    return unwrapBufferHandle(bufferSrc)->copy_to_host(offset, size, pData);
}

AnyDSLResult anydslSynchronizeBuffer(AnyDSLBuffer buffer)
{
    TRACE();
    CHECK_HANDLE_RET(buffer);

    return unwrapBufferHandle(buffer)->device()->sync();
}

// ----------------------------------------- Raw pointers
AnyDSLResult anydslAllocateMemory(AnyDSLDevice device, size_t size, void** pPtr)
{
    TRACE();
    if (size == 0)
        return HANDLE_ERROR(AnyDSL_INVALID_VALUE);
    CHECK_RET_PTR(pPtr);

    if (device == AnyDSL_NULL_HANDLE) {
        CpuPlatform* cpu = (CpuPlatform*)Runtime::instance().host();
        const auto pair  = cpu->host()->allocate_memory(size);
        if (std::get<1>(pair) == nullptr)
            return std::get<0>(pair);

        *pPtr = std::get<1>(pair);
        return std::get<0>(pair);
    } else {
        CHECK_HANDLE_RET(device);

        Device* actualDevice = unwrapDeviceHandle(device);
        const auto pair      = actualDevice->allocate_memory(size);
        if (std::get<1>(pair) == nullptr)
            return std::get<0>(pair);

        *pPtr = std::get<1>(pair);
        return std::get<0>(pair);
    }
}

AnyDSLResult anydslReleaseMemory(AnyDSLDevice device, void* ptr)
{
    TRACE();
    CHECK_RET_PTR(ptr);

    if (device == AnyDSL_NULL_HANDLE) {
        CpuPlatform* cpu = (CpuPlatform*)Runtime::instance().host();
        return cpu->host()->release_memory(ptr);
    } else {
        CHECK_HANDLE_RET(device);

        Device* actualDevice = unwrapDeviceHandle(device);
        return actualDevice->release_memory(ptr);
    }
}

// ----------------------------------------- Events
AnyDSLResult anydslCreateEvent(AnyDSLDevice device, const AnyDSLCreateEventInfo* pInfo, AnyDSLEvent* pEvent)
{
    TRACE();
    // TODO: Host

    CHECK_HANDLE_RET(device);
    CHECK_RET_PTR(pEvent);
    CHECK_RET_PTR(pInfo);
    CHECK_RET_TYPE(pInfo, AnyDSL_STRUCTURE_TYPE_CREATE_EVENT_INFO);

    Device* actualDevice = unwrapDeviceHandle(device);
    const auto pair      = actualDevice->create_event(pInfo);
    if (std::get<1>(pair) == nullptr)
        return std::get<0>(pair);

    *pEvent = (AnyDSLEvent)std::get<1>(pair);
    return std::get<0>(pair);
}

AnyDSLResult anydslDestroyEvent(AnyDSLEvent event)
{
    TRACE();
    CHECK_HANDLE_RET(event);

    Event* actualEvent = unwrapEventHandle(event);
    AnyDSLResult res   = actualEvent->destroy();
    delete actualEvent;
    return res;
}

AnyDSLResult anydslRecordEvent(AnyDSLEvent event)
{
    TRACE();
    CHECK_HANDLE_RET(event);

    return unwrapEventHandle(event)->record();
}

AnyDSLResult anydslQueryEvent(AnyDSLEvent startEvent, AnyDSLEvent endEvent, AnyDSLQueryEventInfo* pInfo)
{
    TRACE();
    CHECK_HANDLE_RET(startEvent);

    if (pInfo != nullptr)
        CHECK_RET_TYPE(pInfo, AnyDSL_STRUCTURE_TYPE_QUERY_EVENT_INFO);

    if (pInfo != nullptr && !checkHandle(endEvent))
        return HANDLE_ERROR(AnyDSL_INVALID_HANDLE);

    Event* ev1 = unwrapEventHandle(startEvent);
    Event* ev2 = unwrapEventHandle(endEvent);

    if (ev2 != nullptr && ev1->device() != ev2->device())
        return HANDLE_ERROR(AnyDSL_DEVICE_MISSMATCH);

    return ev1->query(ev2, pInfo);
}

AnyDSLResult anydslSynchronizeEvent(AnyDSLEvent event)
{
    TRACE();
    CHECK_HANDLE_RET(event);

    return unwrapEventHandle(event)->sync();
}

// ----------------------------------------- JIT
AnyDSLResult anydslCompileJIT(const char* program, size_t count, AnyDSLJITModule* pModule, const AnyDSLJITCompileOptions* pOptions, AnyDSLJITCompileResult* pResult)
{
    TRACE();
    return JIT::compile(program, count, pModule, pOptions, pResult);
}

AnyDSLResult anydslDestroyJITModule(AnyDSLJITModule module)
{
    TRACE();
    return JIT::destroyModule(module);
}

AnyDSLResult anydslFreeJITCompileResult(const AnyDSLJITCompileResult* pResult)
{
    TRACE();
    return JIT::freeCompileResult(pResult);
}

AnyDSLResult anydslLookupJIT(AnyDSLJITModule module, const char* function, AnyDSLJITLookupInfo* pInfo)
{
    TRACE();
    return JIT::lookup(module, function, pInfo);
}

AnyDSLResult anydslLinkJITLibrary(AnyDSLJITModule module, size_t count, const AnyDSLJITLinkInfo* pLinkInfo)
{
    TRACE();
    return JIT::link(module, count, pLinkInfo);
}

// ----------------------------------------- Logging
AnyDSL_runtime_API AnyDSLResult anydslCreateLogReportCallback(const AnyDSLLogReportCallbackCreateInfo* pCreateInfo, AnyDSLLogReportCallback* pCallback)
{
    TRACE();
    return Log::instance().registerHandler(pCreateInfo, pCallback);
}

AnyDSL_runtime_API AnyDSLResult anydslDestroyLogReportCallback(AnyDSLLogReportCallback callback)
{
    TRACE();
    return Log::instance().unregisterHandler(callback);
}

AnyDSL_runtime_API AnyDSLResult anydslLogReportMessage(AnyDSLLogReportLevelFlags flags, const char* pMessage)
{
    TRACE();
    return Log::instance().log(flags, pMessage);
}