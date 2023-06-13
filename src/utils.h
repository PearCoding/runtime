#pragma once

#include "anydsl_runtime.h"
#include "anydsl_runtime_internal_config.h"

namespace AnyDSLInternal {
#ifdef AnyDSL_RUNTIME_DEBUG
#define HANDLE_ERROR(res) AnyDSLInternal::handleError(res, AnyDSL_FUNCTION_NAME, __FILE__, __LINE__)
#else
#define HANDLE_ERROR(res) AnyDSLInternal::handleError(res, AnyDSL_FUNCTION_NAME)
#endif

AnyDSLResult handleError(AnyDSLResult result, const char* func_name, const char* file, int line);
AnyDSLResult handleError(AnyDSLResult result, const char* func_name);

/// @brief Iterate through the chain until type is found.
/// @param ptr A pointer to a structure type with AnyDSLStructureType as its first entry, and a void* pointer as next.
/// @param type The type to search for.
/// @return A pointer to a structure or nullptr if not found.
void* acquireChainEntry(const void* ptr, AnyDSLStructureType type);

/// @brief Check if the given structure type is correct.
/// @param ptr A pointer to a structure type with AnyDSLStructureType as its first entry, and a void* pointer as next.
/// @param type The expected type.
/// @return AnyDSL_SUCCESS if the type matches, AnyDSL_INVALID_POINTER if pointer null or AnyDSL_INVALID_VALUE if incorrect type.
AnyDSLResult expectChainEntry(const void* ptr, AnyDSLStructureType type);

bool checkChainEntry(const void* ptr, AnyDSLStructureType type);

const void* nextChainEntry(const void* ptr);

#define CHECK_RET_TYPE(ptr, type)                                                      \
    if (auto res = AnyDSLInternal::expectChainEntry(ptr, type); res != AnyDSL_SUCCESS) \
    return HANDLE_ERROR(res)

#define CHECK_RET_PTR(ptr) \
    if (ptr == nullptr)    \
    return HANDLE_ERROR(AnyDSL_INVALID_POINTER)

} // namespace AnyDSLInternal
