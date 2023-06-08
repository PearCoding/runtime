#include "cpu_device.h"
#include "cpu_buffer.h"
#include "cpu_event.h"
#include "log.h"
#include "utils.h"

#include <algorithm>
#include <fstream>

#if defined(__APPLE__)
#include <sys/sysctl.h>
#include <sys/types.h>
#elif defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

#if !defined(_WIN32)
#include <unistd.h>
#endif

#include <iostream>

namespace AnyDSLInternal {

#if !defined(_WIN32)
static inline size_t getTotalSystemMemory()
{
    long pages     = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    return pages * page_size;
}
#else
static inline size_t getTotalSystemMemory()
{
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    return status.ullTotalPhys;
}
#endif

AnyDSLResult CpuDevice::init()
{
#if defined(__APPLE__)
    size_t buf_len;
    sysctlbyname("machdep.cpu.brand_string", nullptr, &buf_len, nullptr, 0);
    mName.resize(buf_len, '\0');
    sysctlbyname("machdep.cpu.brand_string", mName.data(), &buf_len, nullptr, 0);
#elif defined(_WIN32)
    HKEY key;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0U, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS)
        error("failed to open processor information registry key");

    DWORD cpu_name_type, cpu_name_size;
    if (RegQueryValueExW(key, L"ProcessorNameString", nullptr, &cpu_name_type, nullptr, &cpu_name_size) != ERROR_SUCCESS)
        error("failed to query processor name string length");

    if (cpu_name_type != REG_SZ)
        error("unexpected type for processor name string");

    int cpu_name_length = cpu_name_size / sizeof(wchar_t);

    std::wstring buffer(cpu_name_length, '\0');
    if (RegQueryValueExW(key, L"ProcessorNameString", nullptr, &cpu_name_type, reinterpret_cast<LPBYTE>(buffer.data()), &cpu_name_size) != ERROR_SUCCESS)
        error("failed to query processor name string");

    RegCloseKey(key);

    int u8_cpu_name_length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, buffer.data(), cpu_name_length, nullptr, 0, nullptr, nullptr);

    if (u8_cpu_name_length <= 0)
        error("failed to compute converted UTF-8 CPU name string length");

    mName.resize(u8_cpu_name_length, '\0');

    if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, buffer.data(), cpu_name_length, mName.data(), u8_cpu_name_length, nullptr, nullptr) <= 0)
        error("failed to convert CPU name string to UTF-8");
#else
    std::ifstream cpuinfo("/proc/cpuinfo");

    if (!cpuinfo)
        error("failed to open /proc/cpuinfo");

#if defined __arm__ || __aarch64__
    std::string model_string = "CPU part\t: ";
#else // x86, x86_64
    std::string model_string = "model name\t: ";
#endif

    std::search(std::istreambuf_iterator<char>(cpuinfo), {}, model_string.begin(), model_string.end());
    std::getline(cpuinfo >> std::ws, mName);
#endif

    return AnyDSL_SUCCESS;
}

AnyDSLResult CpuDevice::get_handle(AnyDSLDeviceHandleInfo* pInfo)
{
    ANYDSL_CHECK_RET_PTR(pInfo);
    ANYDSL_CHECK_RET_TYPE(pInfo, AnyDSL_STRUCTURE_TYPE_DEVICE_HANDLE_INFO);

    pInfo->pHandle = nullptr;

    return AnyDSL_SUCCESS;
}

AnyDSLResult CpuDevice::get_info(AnyDSLDeviceInfo* pInfo)
{
    ANYDSL_CHECK_RET_PTR(pInfo);
    ANYDSL_CHECK_RET_TYPE(pInfo, AnyDSL_STRUCTURE_TYPE_DEVICE_INFO);
    // TODO: Iterate through the chain?

    pInfo->isHost       = AnyDSL_TRUE;
    pInfo->deviceNumber = 0;
    pInfo->deviceType   = AnyDSL_DEVICE_HOST;
    pInfo->name         = mName.c_str();
    pInfo->totalMemory  = getTotalSystemMemory();
    pInfo->version      = 1;

    return AnyDSL_SUCCESS;
}

AnyDSLResult CpuDevice::get_features(AnyDSLDeviceFeatures* pFeatures)
{
    ANYDSL_CHECK_RET_PTR(pFeatures);
    ANYDSL_CHECK_RET_TYPE(pFeatures, AnyDSL_STRUCTURE_TYPE_DEVICE_FEATURES);

    return AnyDSL_SUCCESS;
}

AnyDSLResult CpuDevice::set_options(AnyDSLDeviceOptions* pOptions)
{
    ANYDSL_CHECK_RET_PTR(pOptions);
    ANYDSL_CHECK_RET_TYPE(pOptions, AnyDSL_STRUCTURE_TYPE_DEVICE_OPTIONS);

    return AnyDSL_SUCCESS;
}

AnyDSLResult CpuDevice::sync()
{
    return AnyDSL_SUCCESS;
}

std::tuple<AnyDSLResult, Buffer*> CpuDevice::create_buffer(const AnyDSLCreateBufferInfo* pInfo)
{
    CpuBuffer* buffer = new CpuBuffer(this);

    if (buffer == nullptr)
        return { AnyDSL_OUT_OF_HOST_MEMORY, nullptr };

    AnyDSLResult res = buffer->create(pInfo);
    return { res, buffer };
}

std::tuple<AnyDSLResult, Event*> CpuDevice::create_event(const AnyDSLCreateEventInfo* pInfo)
{
    CpuEvent* event = new CpuEvent(this);

    if (event == nullptr)
        return { AnyDSL_OUT_OF_HOST_MEMORY, nullptr };

    AnyDSLResult res = event->create(pInfo);
    return { res, event };
}

AnyDSLResult CpuDevice::launch_kernel(const AnyDSLLaunchKernelInfo* pInfo)
{
    ANYDSL_CHECK_RET_PTR(pInfo);
    ANYDSL_CHECK_RET_TYPE(pInfo, AnyDSL_STRUCTURE_TYPE_DEVICE_LAUNCH_KERNEL_INFO);

    return AnyDSL_NOT_SUPPORTED;
}
} // namespace AnyDSLInternal