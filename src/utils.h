#pragma once

#include "anydsl_runtime.h"

namespace AnyDSLInternal {
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

#define ANYDSL_CHECK_RET_TYPE(ptr, type)                                               \
    if (auto res = AnyDSLInternal::expectChainEntry(ptr, type); res != AnyDSL_SUCCESS) \
    return res

#define ANYDSL_CHECK_RET_PTR(ptr) \
    if (ptr == nullptr)           \
    return AnyDSL_INVALID_POINTER
} // namespace AnyDSLInternal
