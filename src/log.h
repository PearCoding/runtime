#pragma once

#include "anydsl_runtime.h"
#include "anydsl_runtime_internal_config.h"

#include <vector>

namespace AnyDSLInternal {
class Log {
public:
    AnyDSLResult log(AnyDSLLogReportLevelFlags flags, const char* pMessage);
    AnyDSLResult logf(AnyDSLLogReportLevelFlags flags, const char* pMessage, ...);

    AnyDSLResult registerHandler(const AnyDSLLogReportCallbackCreateInfo* pCreateInfo, AnyDSLLogReportCallback* pCallback);
    AnyDSLResult unregisterHandler(AnyDSLLogReportCallback callback);

    static Log& instance();

private:
    Log();
    ~Log();

    std::vector<AnyDSLLogReportCallbackCreateInfo> mHandlers;
};

inline void unused() {}
template <typename T, typename... Args>
inline void unused(const T& t, Args... args)
{
    (void)t;
    unused(args...);
}

inline void print(AnyDSLLogReportLevelFlags flags, const char* fmt)
{
    Log::instance().log(flags, fmt);
}

template <typename... Args>
inline void print(AnyDSLLogReportLevelFlags flags, const char* fmt, Args... args)
{
    Log::instance().logf(flags, fmt, args...);
}

template <typename... Args>
inline void error(const char* fmt, Args... args)
{
    print(AnyDSL_LOG_REPORT_LEVEL_ERROR_BIT, fmt, args...);
}

template <typename... Args>
inline void warning(const char* fmt, Args... args)
{
    print(AnyDSL_LOG_REPORT_LEVEL_WARNING_BIT, fmt, args...);
}

template <typename... Args>
inline void info(const char* fmt, Args... args)
{
    print(AnyDSL_LOG_REPORT_LEVEL_INFO_BIT, fmt, args...);
}

template <typename... Args>
inline void debug(const char* fmt, Args... args)
{
#ifdef AnyDSL_RUNTIME_ENABLE_DEBUG_OUTPUT
    print(AnyDSL_LOG_REPORT_LEVEL_DEBUG_BIT, fmt, args...);
#else
    unused(fmt, args...);
#endif
}

template <typename... Args>
inline void trace_call(const char* fmt, Args... args)
{
#ifdef AnyDSL_RUNTIME_ENABLE_TRACE_CALLS_OUTPUT
    print(AnyDSL_LOG_REPORT_LEVEL_TRACE_BIT, fmt, args...);
#else
    unused(fmt, args...);
#endif
}

template <typename... Args>
inline void trace_err(const char* fmt, Args... args)
{
#ifdef AnyDSL_RUNTIME_ENABLE_TRACE_ERRORS_OUTPUT
    print(AnyDSL_LOG_REPORT_LEVEL_ERROR_BIT, fmt, args...);
#else
    unused(fmt, args...);
#endif
}
} // namespace AnyDSLInternal
