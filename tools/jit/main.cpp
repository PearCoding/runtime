#include "anydsl_runtime.h"
#include <fstream>
#include <iostream>
#include <sstream>

static void logHandler(AnyDSLLogReportLevelFlags flags,
                       const char* pMessage,
                       void* pUserData)
{
    if (AnyDSL_CHECK_BIT(flags, AnyDSL_LOG_REPORT_LEVEL_DEBUG_BIT))
        std::cout << "DEBUG  : ";
    else if (AnyDSL_CHECK_BIT(flags, AnyDSL_LOG_REPORT_LEVEL_INFO_BIT))
        std::cout << "INFO   : ";
    else if (AnyDSL_CHECK_BIT(flags, AnyDSL_LOG_REPORT_LEVEL_WARNING_BIT))
        std::cout << "WARNING: ";
    else
        std::cout << "ERROR  : ";

    std::cout << pMessage << std::endl;
}

int main(int argc, char** argv)
{
    AnyDSLLogReportCallbackCreateInfo logInfo {
        AnyDSL_STRUCTURE_TYPE_LOG_REPORT_CALLBACK_CREATE_INFO,
        nullptr,
        AnyDSL_LOG_REPORT_LEVEL_MAX_ENUM,
        logHandler,
        nullptr
    };
    AnyDSLLogReportCallback logClb;
    anydslCreateLogReportCallback(&logInfo, &logClb);

    AnyDSLFeatures features = {
        AnyDSL_STRUCTURE_TYPE_FEATURES,
        nullptr
    };
    if (anydslGetFeatures(&features) != AnyDSL_SUCCESS) {
        std::cout << "Could not query features of AnyDSL" << std::endl;
        return -1;
    }

    if (features.hasJIT != AnyDSL_TRUE) {
        std::cout << "Your AnyDSL runtime does has no JIT support" << std::endl;
        return -2;
    }

    if (argc < 3) {
        std::cout << "Not enough arguments given. Expected <function> <files>..." << std::endl;
        return -1;
    }

    std::string func_name = argv[1];

    std::stringstream src;
    for (int c = 2; c < argc; ++c) {
        std::ifstream stream(argv[c]);
        if (!stream) {
            std::cout << "Could not open '" << argv[c] << "'" << std::endl;
            return -1;
        }
        src << stream.rdbuf();
    }

    std::string content = src.str();
    if (content.empty()) {
        std::cout << "Given source code is empty" << std::endl;
        return -1;
    }

    AnyDSLJITCompileOptions options = {
        AnyDSL_STRUCTURE_TYPE_JIT_COMPILE_OPTIONS,
        nullptr,
        3,
        4,
        AnyDSL_COMPILE_LANGUAGE_ARTIC_BIT,
        AnyDSL_TRUE,
        nullptr
    };

    AnyDSLJITCompileResult result = {
        AnyDSL_STRUCTURE_TYPE_JIT_COMPILE_RESULT,
        nullptr,
        nullptr // Will be set by function
    };

    AnyDSLJITModule module;
    AnyDSLResult res = anydslCompileJIT(content.data(), content.size(), &module, &options, &result);

    if (result.logOutput != nullptr)
        std::cout << result.logOutput << std::endl;
    anydslFreeJITCompileResult(&result);

    if (res != AnyDSL_SUCCESS) {
        std::cout << "Failed to compile" << std::endl;
        return -1;
    }

    AnyDSLJITLookupInfo info = {
        AnyDSL_STRUCTURE_TYPE_JIT_LOOKUP_INFO,
        nullptr,
        nullptr // Will be set by function
    };

    if (anydslLookupJIT(module, func_name.c_str(), &info) != AnyDSL_SUCCESS) {
        std::cout << "Failed to get function '" << func_name << "'" << std::endl;
        anydslDestroyJITModule(module);
        return -1;
    }

    if (info.pHandle == nullptr) {
        std::cout << "anydslLookupJIT should never return a zero function and AnyDSL_SUCCESS!" << std::endl;
        anydslDestroyJITModule(module);
        return -42;
    }

    auto func = (void (*)())info.pHandle;
    func();

    anydslDestroyJITModule(module);
    return 0;
}