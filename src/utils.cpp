#include "utils.h"
#include "log.h"

extern "C" {
typedef struct AnyDSLRawStructure {
    AnyDSLStructureType sType;
    const void* pNext;
} AnyDSLRawStructure;
}

namespace AnyDSLInternal {
static const char* resultToString(AnyDSLResult result)
{
    switch (result) {
    default:
        return "Unknown";
    case AnyDSL_INCOMPLETE:
        return "Incomplete";
    case AnyDSL_NOT_AVAILABLE:
        return "Not available";
    case AnyDSL_NOT_SUPPORTED:
        return "Not supported";
    case AnyDSL_OUT_OF_HOST_MEMORY:
        return "Out of host memory";
    case AnyDSL_OUT_OF_DEVICE_MEMORY:
        return "Out of device memory";
    case AnyDSL_INVALID_POINTER:
        return "Invalid pointer";
    case AnyDSL_INVALID_VALUE:
        return "Invalid value";
    case AnyDSL_INVALID_HANDLE:
        return "Invalid handle";
    case AnyDSL_DEVICE_MISSMATCH:
        return "Device missmatch";
    case AnyDSL_PLATFORM_ERROR:
        return "Platform error";
    case AnyDSL_JIT_ERROR:
        return "JIT error";
    case AnyDSL_JIT_NO_FUNCTION:
        return "JIT no function";
    }
}

AnyDSLResult handleError(AnyDSLResult result, const char* func_name, const char* file, int line)
{
    if (result < 0)
        trace_err("Function %s [file %s, line %d] error: '%s'", func_name, file, line, resultToString(result));

    return result;
}

AnyDSLResult handleError(AnyDSLResult result, const char* func_name)
{
    if (result < 0)
        trace_err("Function %s error: '%s'", func_name, resultToString(result));

    return result;
}

void* acquireChainEntry(const void* ptr, AnyDSLStructureType type)
{
    AnyDSLRawStructure* entry = (AnyDSLRawStructure*)ptr;
    while (entry != nullptr) {
        if (entry->sType == type)
            break;
        entry = (AnyDSLRawStructure*)entry->pNext;
    }
    return entry;
}

AnyDSLResult expectChainEntry(const void* ptr, AnyDSLStructureType type)
{
    AnyDSLRawStructure* entry = (AnyDSLRawStructure*)ptr;

    if (entry == nullptr)
        return AnyDSL_INVALID_POINTER;
    else
        return entry->sType == type ? AnyDSL_SUCCESS : AnyDSL_INVALID_VALUE;
}

bool checkChainEntry(const void* ptr, AnyDSLStructureType type)
{
    return expectChainEntry(ptr, type) == AnyDSL_SUCCESS;
}

const void* nextChainEntry(const void* ptr)
{
    AnyDSLRawStructure* entry = (AnyDSLRawStructure*)ptr;
    if (entry != nullptr)
        return entry->pNext;
    return nullptr;
}

} // namespace AnyDSLInternal
