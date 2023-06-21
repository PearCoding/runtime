#ifndef ANYDSL_RUNTIME_H
#define ANYDSL_RUNTIME_H

#ifndef ANYDSL_NO_STD
#include <stddef.h>
#include <stdint.h>
#endif

#ifndef ANYDSL_USE_CUSTOM_CONFIG
#include "anydsl_runtime_config.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// New interface (2.0) inspired by Vulkan.
// This interface is designed to be used by host code only.

#define AnyDSL_CHECK_BIT(bits, flag) (((bits) & (flag)) == (flag))

#define ANYDSL_MAKE_HANDLE(object) typedef struct object##_T* object

ANYDSL_MAKE_HANDLE(AnyDSLDevice);
ANYDSL_MAKE_HANDLE(AnyDSLBuffer);
ANYDSL_MAKE_HANDLE(AnyDSLEvent);
ANYDSL_MAKE_HANDLE(AnyDSLJITModule);
ANYDSL_MAKE_HANDLE(AnyDSLLogReportCallback);

#undef ANYDSL_MAKE_HANDLE

#define AnyDSL_NULL_HANDLE (0)

#define AnyDSL_FALSE (0)
#define AnyDSL_TRUE (1)

typedef uint32_t AnyDSLBool;
typedef uint32_t AnyDSLFlags;
typedef uint64_t AnyDSLDeviceSize;
typedef uint64_t AnyDSLDevicePointer;

typedef enum AnyDSLResult {
    AnyDSL_SUCCESS              = 0,
    AnyDSL_NOT_READY            = 1,
    AnyDSL_INCOMPLETE           = -1,
    AnyDSL_NOT_AVAILABLE        = -2,
    AnyDSL_NOT_SUPPORTED        = -3,
    AnyDSL_OUT_OF_HOST_MEMORY   = -4,
    AnyDSL_OUT_OF_DEVICE_MEMORY = -5,
    AnyDSL_INVALID_POINTER      = -6,
    AnyDSL_INVALID_VALUE        = -7,
    AnyDSL_INVALID_HANDLE       = -8,
    AnyDSL_DEVICE_MISSMATCH     = -9,
    AnyDSL_PLATFORM_ERROR       = -10,
    AnyDSL_JIT_ERROR            = -100,
    AnyDSL_JIT_NO_FUNCTION      = -101,
} AnyDSLResult;

typedef enum AnyDSLDeviceType {
    AnyDSL_DEVICE_HOST   = 0,
    AnyDSL_DEVICE_CUDA   = 1,
    AnyDSL_DEVICE_OPENCL = 2,
    AnyDSL_DEVICE_HSA    = 3,
    // @Add yours
} AnyDSLDeviceType;

typedef enum AnyDSLStructureType {
    AnyDSL_STRUCTURE_TYPE_FEATURES    = 0x0,
    AnyDSL_STRUCTURE_TYPE_DEVICE_INFO = 0x1,
    AnyDSL_STRUCTURE_TYPE_OPTIONS     = 0x2,

    AnyDSL_STRUCTURE_TYPE_GET_DEVICE_REQUEST        = 0x10,
    AnyDSL_STRUCTURE_TYPE_DEVICE_HANDLE_INFO        = 0x11,
    AnyDSL_STRUCTURE_TYPE_DEVICE_FEATURES           = 0x12,
    AnyDSL_STRUCTURE_TYPE_DEVICE_OPTIONS            = 0x13,
    AnyDSL_STRUCTURE_TYPE_DEVICE_LAUNCH_KERNEL_INFO = 0x1F,

    AnyDSL_STRUCTURE_TYPE_CREATE_BUFFER_INFO      = 0x20,
    AnyDSL_STRUCTURE_TYPE_GET_BUFFER_POINTER_INFO = 0x21,

    AnyDSL_STRUCTURE_TYPE_CREATE_EVENT_INFO = 0x30,
    AnyDSL_STRUCTURE_TYPE_QUERY_EVENT_INFO  = 0x31,

    AnyDSL_STRUCTURE_TYPE_JIT_COMPILE_OPTIONS = 0x100,
    AnyDSL_STRUCTURE_TYPE_JIT_COMPILE_RESULT  = 0x101,
    AnyDSL_STRUCTURE_TYPE_JIT_LOOKUP_INFO     = 0x102,
    AnyDSL_STRUCTURE_TYPE_JIT_LINK_INFO       = 0x103,

    AnyDSL_STRUCTURE_TYPE_LOG_REPORT_CALLBACK_CREATE_INFO = 0x1000,

    AnyDSL_STRUCTURE_TYPE_DEVICE_FEATURES_CUDA = 0x10000,
    AnyDSL_STRUCTURE_TYPE_DEVICE_OPTIONS_CUDA  = 0x10001,
} AnyDSLStructureType;

typedef enum AnyDSLLogReportLevelFlagBits {
    AnyDSL_LOG_REPORT_LEVEL_ERROR_BIT   = 0x001,
    AnyDSL_LOG_REPORT_LEVEL_WARNING_BIT = 0x002,
    AnyDSL_LOG_REPORT_LEVEL_INFO_BIT    = 0x004,
    AnyDSL_LOG_REPORT_LEVEL_DEBUG_BIT   = 0x008,
    AnyDSL_LOG_REPORT_LEVEL_TRACE_BIT   = 0x010,
    AnyDSL_LOG_REPORT_LEVEL_MAX_ENUM    = 0x7FFFFFFF
} AnyDSLLogReportLevelFlagBits;
typedef AnyDSLFlags AnyDSLLogReportLevelFlags;

typedef void(ANYDSL_API_PTR* PFN_anydslLogReportCallback)(
    AnyDSLLogReportLevelFlags flags,
    const char* pMessage,
    void* pUserData);

typedef enum AnyDSLCreateBufferFlagBits {
    AnyDSL_CREATE_BUFFER_HOST_BIT    = 0x1, // TODO: Add flag showing this is supported
    AnyDSL_CREATE_BUFFER_MANAGED_BIT = 0x2, // TODO: Add flag showing this is supported
    AnyDSL_CREATE_BUFFER_MAX_ENUM    = 0x7FFFFFFF
} AnyDSLCreateBufferFlagBits;
typedef AnyDSLFlags AnyDSLCreateBufferFlags;

typedef enum AnyDSLCompileLanguageFlagBits {
    AnyDSL_COMPILE_LANGUAGE_ARTIC_BIT  = 0x1,
    AnyDSL_COMPILE_LANGUAGE_IMPALA_BIT = 0x2,
    AnyDSL_COMPILE_LANGUAGE_MAX_ENUM   = 0x7FFFFFFF
} AnyDSLCompileLanguageFlagBits;
typedef AnyDSLFlags AnyDSLCompileLanguageFlags;

// ###################################### Structures

// -------------------------------------- Structures [Runtime]
typedef struct AnyDSLVersion {
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
} AnyDSLVersion;

typedef struct AnyDSLFeatures {
    AnyDSLStructureType sType;
    const void* pNext;

    AnyDSLBool hasJIT;
    AnyDSLCompileLanguageFlags supportedLanguages;
} AnyDSLFeatures;

typedef struct AnyDSLOptions {
    AnyDSLStructureType sType;
    const void* pNext;

    const char* globalCacheDir; // The global cache dir
} AnyDSLOptions;

// -------------------------------------- Structures [Device]
typedef struct AnyDSLDeviceInfo {
    AnyDSLStructureType sType;
    const void* pNext;

    AnyDSLDeviceType deviceType;
    uint32_t deviceNumber;
    const char* name;
    uint32_t version;
    AnyDSLBool isHost;

    size_t totalMemory; // In bytes
} AnyDSLDeviceInfo;

typedef struct AnyDSLGetDeviceRequest {
    AnyDSLStructureType sType;
    const void* pNext;

    AnyDSLDeviceType deviceType;
    uint32_t deviceNumber;
} AnyDSLGetDeviceRequest;

typedef struct AnyDSLDeviceHandleInfo {
    AnyDSLStructureType sType;
    const void* pNext;

    void* pHandle; // Device specific handle.
} AnyDSLGetHandleInfo;

typedef struct AnyDSLDeviceFeatures {
    AnyDSLStructureType sType;
    const void* pNext;
} AnyDSLDeviceFeatures;

typedef struct AnyDSLDeviceOptions {
    AnyDSLStructureType sType;
    const void* pNext;
    // Highly device specific
} AnyDSLDeviceOptions;

typedef struct AnyDSLLaunchKernelInfo {
    AnyDSLStructureType sType;
    const void* pNext;

    const char* file_name;
    const char* kernel_name;
    size_t grid[3];
    size_t block[3];

    void** kernelParams;
} AnyDSLLaunchKernelInfo;

// -------------------------------------- Structures [Buffer]
typedef struct AnyDSLCreateBufferInfo {
    AnyDSLStructureType sType;
    const void* pNext;

    AnyDSLDeviceSize size;
    AnyDSLCreateBufferFlags flags;
} AnyDSLCreateBufferInfo;

typedef struct AnyDSLBufferCopy {
    AnyDSLDeviceSize offsetSrc;
    AnyDSLDeviceSize offsetDst;
    AnyDSLDeviceSize size;
} AnyDSLBufferCopy;

typedef struct AnyDSLGetBufferPointerInfo {
    AnyDSLStructureType sType;
    const void* pNext;
    AnyDSLDevicePointer hostPointer;
    AnyDSLDevicePointer devicePointer;
} AnyDSLGetBufferPointerInfo;

// -------------------------------------- Structures [Event]
typedef struct AnyDSLCreateEventInfo {
    AnyDSLStructureType sType;
    const void* pNext;
} AnyDSLCreateEventInfo;

typedef struct AnyDSLQueryEventInfo {
    AnyDSLStructureType sType;
    const void* pNext;

    float elapsedTimeMS;
} AnyDSLQueryEventInfo;

// -------------------------------------- Structures [JIT]
typedef struct AnyDSLJITCreateModuleInfo {
    AnyDSLStructureType sType;
    const void* pNext;
} AnyDSLJITCreateModuleInfo;

typedef struct AnyDSLJITCompileOptions {
    AnyDSLStructureType sType;
    const void* pNext;

    /// @brief opt level (0=debug, 1,2,3... higher means more optimization)
    uint32_t optLevel;
    /// @brief log level (4=error only, 3=warn, 2=info, 1=verbose, 0=debug)
    uint32_t logLevel;

    /// @brief Selected language. Only one of the supported allowed.
    AnyDSLCompileLanguageFlags language;

    /// @brief Set to AnyDSL_TRUE to enable caching.
    AnyDSLBool bUseCache;
    /// @brief Directory to cache output. Use NULL for default.
    const char* cacheDir;
} AnyDSLJITCompileOptions;

typedef struct AnyDSLJITCompileResult {
    AnyDSLStructureType sType;
    const void* pNext;

    char* logOutput;
} AnyDSLJITCompileResult;

typedef struct AnyDSLJITLookupInfo {
    AnyDSLStructureType sType;
    const void* pNext;

    const void* pHandle;
} AnyDSLJITLookupInfo;

typedef struct AnyDSLJITLinkInfo {
    AnyDSLStructureType sType;
    const void* pNext;

    const char* libraryFilename;
} AnyDSLJITLinkInfo;

// -------------------------------------- Structures [Utils]
typedef struct AnyDSLLogReportCallbackCreateInfo {
    AnyDSLStructureType sType;
    const void* pNext;
    AnyDSLLogReportLevelFlags flags;
    PFN_anydslLogReportCallback pfnCallback;
    void* pUserData;
} AnyDSLLogReportCallbackCreateInfo;

// ###################################### Functions

// -------------------------------------- Functions [Runtime]
/// @brief Get version of this AnyDSL implementation.
/// @return AnyDSL_SUCCESS if sucessful.
AnyDSL_runtime_API AnyDSLResult anydslGetVersion(AnyDSLVersion* pVersion);
/// @brief Get features of this AnyDSL implementation.
/// @return AnyDSL_SUCCESS if sucessful.
AnyDSL_runtime_API AnyDSLResult anydslGetFeatures(AnyDSLFeatures* pFeatures);
/// @brief Enumerate through all available device, beside the host. Set pInfo = NULL to acquire number of devices available, else pInfo should be an array of AnyDSLDeviceInfo.
/// @return AnyDSL_SUCCESS if sucessful.
AnyDSL_runtime_API AnyDSLResult anydslEnumerateDevices(size_t* pCount, AnyDSLDeviceInfo* pInfo);
/// @brief Set features and options of the general runtime.
/// @return AnyDSL_SUCCESS if sucessful.
AnyDSL_runtime_API AnyDSLResult anydslSetOptions(const AnyDSLOptions* pOptions);

// -------------------------------------- Functions [Device]
/// @brief Get a device for AnyDSL. Keep in mind that this is only to access information, not to gain control over the device.
/// @return AnyDSL_SUCCESS if sucessful.
AnyDSL_runtime_API AnyDSLResult anydslGetDevice(const AnyDSLGetDeviceRequest* pCreateInfo, AnyDSLDevice* pDevice);
/// @brief Get internal handle of the device. This might be a CUDA context or similar depending on the device.
/// @return AnyDSL_SUCCESS if sucessful.
AnyDSL_runtime_API AnyDSLResult anydslGetDeviceHandle(AnyDSLDevice device, AnyDSLDeviceHandleInfo* pHandleInfo);
/// @brief Get information of the device. Same as anydslEnumerateDevices but only for this device.
/// @return AnyDSL_SUCCESS if sucessful.
AnyDSL_runtime_API AnyDSLResult anydslGetDeviceInfo(AnyDSLDevice device, AnyDSLDeviceInfo* pDeviceInfo);
/// @brief Get features and limits of the device if available.
/// @return AnyDSL_SUCCESS if sucessful.
AnyDSL_runtime_API AnyDSLResult anydslGetDeviceFeatures(AnyDSLDevice device, AnyDSLDeviceFeatures* pDeviceFeatures);
/// @brief Set features and options of the device. This changes the properties of all instances of this device!
/// @return AnyDSL_SUCCESS if sucessful.
AnyDSL_runtime_API AnyDSLResult anydslSetDeviceOptions(AnyDSLDevice device, const AnyDSLDeviceOptions* pDeviceOptions);
/// @brief Wait for all operations on a device to finish.
/// @param device The device to wait for.
/// @return AnyDSL_SUCCESS if sucessful.
AnyDSL_runtime_API AnyDSLResult anydslSynchronizeDevice(AnyDSLDevice device);

/// @brief Launch given kernel. The call might be asynchronous depending on the device.
/// @note This function is intended mostly for internal use with other frontends.
/// @param device The device the kernel is assigned with.
/// @param pInfo Info structure containing information regarding the kernel. This is highly device specific.
/// @return AnyDSLResult if the kernel was launched.
AnyDSL_runtime_API AnyDSLResult anydslLaunchKernel(AnyDSLDevice device, const AnyDSLLaunchKernelInfo* pInfo);

// -------------------------------------- Functions [Buffer]
/// @brief  Create a buffer on the given device
/// @param device The device this buffer is bounded to. Use AnyDSL_NULL_HANDLE if a managed buffer on the host is desired.
/// @param pInfo The info structure describing the size and features of the buffer.
/// @param pBuffer Pointer to the new created buffer.
/// @return AnyDSL_SUCCESS if sucessful.
AnyDSL_runtime_API AnyDSLResult anydslCreateBuffer(AnyDSLDevice device, const AnyDSLCreateBufferInfo* pInfo, AnyDSLBuffer* pBuffer);
/// @brief Destroy a buffer previously created by anydslCreateBuffer.
/// @param buffer The buffer to destroy.
/// @return AnyDSL_SUCCESS if sucessful.
AnyDSL_runtime_API AnyDSLResult anydslDestroyBuffer(AnyDSLBuffer buffer);
/// @brief Get device specific pointer to the buffer. The returned pointer is only valid for the device.
/// @param buffer The buffer.
/// @param pInfo Structure containing the device specific pointer and other information.
/// @return AnyDSL_SUCCESS if sucessful.
AnyDSL_runtime_API AnyDSLResult anydslGetBufferPointer(AnyDSLBuffer buffer, AnyDSLGetBufferPointerInfo* pInfo);
/// @brief Asynchronous copy of a buffer.
/// @param bufferSrc The buffer to copy from. Can be the same device as bufferDst or the host.
/// @param bufferDst The buffer to copy to. Can be the same device as bufferSrc or the host.
/// @param count Number of regions to copy.
/// @param pRegions An array of regions to copy.
/// @return AnyDSL_SUCCESS if sucessful.
AnyDSL_runtime_API AnyDSLResult anydslCopyBuffer(AnyDSLBuffer bufferSrc, AnyDSLBuffer bufferDst, uint32_t count, const AnyDSLBufferCopy* pRegions);
/// @brief Asynchronously fill given buffer with a user given value.
/// @param bufferDst The buffer the value will be written to.
/// @param offset Offset inside the buffer.
/// @param count Number of elements to fill the region with. Keep in mind that an element this is not one byte, but four.
/// @param data User given 4 byte value.
/// @return AnyDSL_SUCCESS if sucessful.
AnyDSL_runtime_API AnyDSLResult anydslFillBuffer(AnyDSLBuffer bufferDst, AnyDSLDeviceSize offset, AnyDSLDeviceSize count, uint32_t data);
/// @brief Asynchronously copy the buffer from the host.
/// @param bufferDst The buffer the data will be copied to.
/// @param offset Offset inside the buffer.
/// @param size Size of the region to copy. For proper performance a multiple of 4 is recommend.
/// @param pData Pointer to a 4 byte aligned memory.
/// @return AnyDSL_SUCCESS if sucessful.
AnyDSL_runtime_API AnyDSLResult anydslCopyBufferFromHost(AnyDSLBuffer bufferDst, AnyDSLDeviceSize offset, AnyDSLDeviceSize size, const void* pData);
/// @brief Asynchronously copy the buffer to the host.
/// @param bufferSrc The buffer the data will be copied from.
/// @param offset Offset inside the buffer.
/// @param size Size of the region to copy. For proper performance a multiple of 4 is recommend.
/// @param pData Pointer to a 4 byte aligned memory.
/// @return AnyDSL_SUCCESS if sucessful.
AnyDSL_runtime_API AnyDSLResult anydslCopyBufferToHost(AnyDSLBuffer bufferSrc, AnyDSLDeviceSize offset, AnyDSLDeviceSize size, void* pData);
/// @brief Wait for all operations on the device assosciated with the buffer to finish.
/// @param buffer The buffer whom device will be waited for.
/// @return AnyDSL_SUCCESS if sucessful.
/// @note This will wait for all operations on the device! Not only the pending buffer operations affecting the buffer.
AnyDSL_runtime_API AnyDSLResult anydslSynchronizeBuffer(AnyDSLBuffer buffer);

// -------------------------------------- Raw allocations
/// @brief Allocates memory on the device. This is the same as anydslCreateBuffer (flags=0) but returning a raw pointer.
/// @param device The device this buffer is bounded to. Use AnyDSL_NULL_HANDLE if a managed buffer on the host is desired.
/// @param size The size of the memory block.
/// @param pPtr Pointer to the pointer pointing at the new memory block.
/// @return AnyDSL_SUCCESS if sucessful.
/// @note If possible, use the more secure anydslCreateBuffer interface.
AnyDSL_runtime_API AnyDSLResult anydslAllocateMemory(AnyDSLDevice device, size_t size, void** pPtr);
/// @brief Releases a pointer previously created by anydslAllocateMemory.
/// @param device The device the memory was allocated from. If the device does not match the one which allocated the memory the behaviour is undefined.
/// @param ptr Pointer to the start of the memory block which is to be released.
/// @return AnyDSL_SUCCESS if sucessful.
AnyDSL_runtime_API AnyDSLResult anydslReleaseMemory(AnyDSLDevice device, void* ptr);

// -------------------------------------- Functions [Event]
/// @brief Create event on a given device.
/// @param device The device the event is bounded to.
/// @param pInfo The info structure describing requested flags and other custom information of the to be created event.
/// @param pEvent Pointer to the newly created event.
/// @return AnyDSL_SUCCESS if sucessful.
AnyDSL_runtime_API AnyDSLResult anydslCreateEvent(AnyDSLDevice device, const AnyDSLCreateEventInfo* pInfo, AnyDSLEvent* pEvent);
/// @brief Destroy an event previously created by anydslCreateEvent.
/// @param event The event to destroy.
/// @return AnyDSL_SUCCESS if sucessful.
AnyDSL_runtime_API AnyDSLResult anydslDestroyEvent(AnyDSLEvent event);
/// @brief Record an event.
/// @param event The event to record.
/// @return AnyDSL_SUCCESS if sucessful.
AnyDSL_runtime_API AnyDSLResult anydslRecordEvent(AnyDSLEvent event);
/// @brief Query information of two events.
/// @param startEvent The starting event to query. Both events have to be created by the same device.
/// @param endEvent The ending event to query. Both events have to be created by the same device. This can be AnyDSL_NULL_HANDLE only if pInfo is null. This is to permit querying information about startEvent only.
/// @param pInfo Optional information when event has already completed. Set to NULL if not desired. Only populated if event has finished.
/// @return AnyDSL_SUCCESS if the event finished, AnyDSL_NOT_READY if not. Every other return value indicates an error.
AnyDSL_runtime_API AnyDSLResult anydslQueryEvent(AnyDSLEvent startEvent, AnyDSLEvent endEvent, AnyDSLQueryEventInfo* pInfo);
/// @brief Wait on the host for the event to finish.
/// @param event Event to wait for on the host.
/// @return AnyDSL_SUCCESS if sucessful.
AnyDSL_runtime_API AnyDSLResult anydslSynchronizeEvent(AnyDSLEvent event);

// -------------------------------------- Functions [JIT]
/// @brief Compile a program and create a jit module for it.
/// @param program A string containing the actual program.
/// @param size Size of the string containing the program.
/// @param pModule Pointer to the newly created jit module. Should be freed with anydslDestroyJITModule.
/// @param pOptions Pointer to a structure containing options and properties.
/// @param pResult Optional pointer to a structure containing additional information. Set it to NULL if not desired. Has to be freed with anydslFreeJITCompileResult if used.
/// @return AnyDSL_SUCCESS if sucessful.
AnyDSL_runtime_API AnyDSLResult anydslCompileJIT(const char* program, size_t size, AnyDSLJITModule* pModule, const AnyDSLJITCompileOptions* pOptions, AnyDSLJITCompileResult* pResult);
/// @brief Destroy a jit module previously created by anydslCompileJIT.
/// @return AnyDSL_SUCCESS if sucessful.
AnyDSL_runtime_API AnyDSLResult anydslDestroyJITModule(AnyDSLJITModule module);
/// @brief Free a AnyDSLJITCompileResult acquired by anydslCompileJIT.
/// @return AnyDSL_SUCCESS if sucessful.
AnyDSL_runtime_API AnyDSLResult anydslFreeJITCompileResult(const AnyDSLJITCompileResult* pResult);
/// @brief Lookup a function inside a jit module and return it via pInfo. The given function name has to be a null-terminated string.
/// @return AnyDSL_SUCCESS if sucessful.
AnyDSL_runtime_API AnyDSLResult anydslLookupJIT(AnyDSLJITModule module, const char* function, AnyDSLJITLookupInfo* pInfo);
/// @brief Link an external library to the jit module. Due to the unfortunate nature of the internal machine runtime, this might affect other jit modules as well.
/// @param module Currently unnused.
/// @param count Number of AnyDSLJITLinkInfo entries.
/// @param pLinkInfo An array of libraries to link to.
/// @return AnyDSL_SUCCESS if sucessful.
AnyDSL_runtime_API AnyDSLResult anydslLinkJITLibrary(AnyDSLJITModule module, size_t count, const AnyDSLJITLinkInfo* pLinkInfo);

// TODO: Dump binary from jit module, allow loading etc

// -------------------------------------- Functions [Utils]
/// @brief Attach the newly created log report handler to the runtime.
/// @param pCreateInfo Information and settings regarding the new handler.
/// @param pCallback Pointer to the new handle this report handler is associated with.
/// @return AnyDSL_SUCCESS if sucessful.
AnyDSL_runtime_API AnyDSLResult anydslCreateLogReportCallback(const AnyDSLLogReportCallbackCreateInfo* pCreateInfo, AnyDSLLogReportCallback* pCallback);
/// @brief Remove handler previously attached with anydslCreateLogReportCallback from the runtime.
/// @param callback The handle to remove.
/// @return AnyDSL_SUCCESS if sucessful.
AnyDSL_runtime_API AnyDSLResult anydslDestroyLogReportCallback(AnyDSLLogReportCallback callback);
/// @brief Submit a message into the runtime, allowing previously registered handlers to handle it.
/// @param flags The flags associated with the message.
/// @param region The region or object this message is associated with.
/// @param pMessage Tha actual message. Should be English for brevity.
/// @return AnyDSL_SUCCESS if sucessful.
AnyDSL_runtime_API AnyDSLResult anydslLogReportMessage(AnyDSLLogReportLevelFlags flags, const char* pMessage);

// TODO: Some of the functions of the old interface are not mapped yet.
#ifdef __cplusplus
}
#endif

#include "extensions/anydsl_runtime_cuda.h"

#endif // ANYDSL_RUNTIME_H
