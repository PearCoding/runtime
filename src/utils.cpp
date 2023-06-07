#include "utils.h"

extern "C" {
typedef struct AnyDSLRawStructure {
    AnyDSLStructureType sType;
    const void* pNext;
} AnyDSLRawStructure;
}

namespace AnyDSLInternal {
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
