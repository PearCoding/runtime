#include "log.h"
#include "utils.h"

#include <cstdarg>
#include <cstdio>

namespace AnyDSLInternal {
Log::Log()
{
}

Log::~Log()
{
}

// TODO: Thread safe?
AnyDSLResult Log::log(AnyDSLLogReportLevelFlags flags, const char* pMessage)
{
    ANYDSL_CHECK_RET_PTR(pMessage);
    if ((flags & AnyDSL_LOG_REPORT_LEVEL_MAX_ENUM) == 0)
        return AnyDSL_INVALID_VALUE;

    for (size_t i = 0; i < mHandlers.size(); ++i) {
        const auto& handler = mHandlers[i];
        if (handler.pfnCallback == nullptr) // Disabled
            continue;

        if ((handler.flags & flags) == flags)
            handler.pfnCallback(flags, pMessage, handler.pUserData);
    }

    return AnyDSL_SUCCESS;
}

// Only used internally
AnyDSLResult Log::logf(AnyDSLLogReportLevelFlags flags, const char* pMessage, ...)
{
    constexpr size_t BufferSize = 1024;
    thread_local char buffer[BufferSize];

    va_list args;
    va_start(args, pMessage);
    vsnprintf(buffer, BufferSize, pMessage, args);

    buffer[BufferSize - 1] = 0; // Enforce null terminator

    const auto res = log(flags, buffer);
    va_end(args);

    return res;
}

AnyDSLResult Log::registerHandler(const AnyDSLLogReportCallbackCreateInfo* pCreateInfo, AnyDSLLogReportCallback* pCallback)
{
    ANYDSL_CHECK_RET_PTR(pCallback);
    ANYDSL_CHECK_RET_TYPE(pCreateInfo, AnyDSL_STRUCTURE_TYPE_LOG_REPORT_CALLBACK_CREATE_INFO);

    if (pCreateInfo->pfnCallback == nullptr)
        return AnyDSL_INVALID_VALUE;

    if ((pCreateInfo->flags & AnyDSL_LOG_REPORT_LEVEL_MAX_ENUM) == 0)
        return AnyDSL_INVALID_VALUE;

    size_t id = mHandlers.size() + 1;
    mHandlers.emplace_back(*pCreateInfo);

    *pCallback = (AnyDSLLogReportCallback)id;
    return AnyDSL_SUCCESS;
}

AnyDSLResult Log::unregisterHandler(AnyDSLLogReportCallback callback)
{
    uintptr_t idx = (uintptr_t)callback - 1;
    if (idx < (uintptr_t)mHandlers.size()) {
        // We can not destroy the vector, so just disable the handler.
        mHandlers.at(idx).pfnCallback = nullptr;
        return AnyDSL_SUCCESS;
    }

    return AnyDSL_INVALID_HANDLE;
}

Log& Log::instance()
{
    static Log sLog;
    return sLog;
}

} // namespace AnyDSLInternal