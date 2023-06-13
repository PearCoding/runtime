#include "jit.h"
#include "log.h"

// NOTE: This file is only compiled if jit is turned OFF

namespace AnyDSLInternal {
AnyDSLResult JIT::compile(const char* program, size_t size, AnyDSLJITModule* pModule, const AnyDSLJITCompileOptions* pOptions, AnyDSLJITCompileResult* pResult)
{
    unused(program, size, pModule, pOptions, pResult);
    return HANDLE_ERROR(AnyDSL_NOT_SUPPORTED);
}

AnyDSLResult JIT::destroyModule(AnyDSLJITModule module)
{
    unused(module);
    return HANDLE_ERROR(AnyDSL_NOT_SUPPORTED);
}

AnyDSLResult JIT::freeCompileResult(const AnyDSLJITCompileResult* pResult)
{
    unused(pResult);
    return HANDLE_ERROR(AnyDSL_NOT_SUPPORTED);
}

AnyDSLResult JIT::lookup(AnyDSLJITModule module, const char* function, AnyDSLJITLookupInfo* pInfo)
{
    unused(module, function, pInfo);
    return HANDLE_ERROR(AnyDSL_NOT_SUPPORTED);
}

AnyDSLResult JIT::link(AnyDSLJITModule module, size_t count, const AnyDSLJITLinkInfo* pLinkInfo)
{
    unused(module, count, pLinkInfo);
    return HANDLE_ERROR(AnyDSL_NOT_SUPPORTED);
}

} // namespace AnyDSLInternal
