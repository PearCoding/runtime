#pragma once

#include "anydsl_runtime.h"

#include <utility>

namespace AnyDSLInternal {
class JIT {
public:
    static AnyDSLResult compile(const char* program, size_t size, AnyDSLJITModule* pModule, const AnyDSLJITCompileOptions* pProperties, AnyDSLJITCompileResult* pResult);
    static AnyDSLResult destroyModule(AnyDSLJITModule module);
    static AnyDSLResult freeCompileResult(const AnyDSLJITCompileResult* pResult);
    static AnyDSLResult lookup(AnyDSLJITModule module, const char* function, AnyDSLJITLookupInfo* pInfo);
    static AnyDSLResult link(AnyDSLJITModule module, size_t count, const AnyDSLJITLinkInfo* pLinkInfo);
};
} // namespace AnyDSLInternal
