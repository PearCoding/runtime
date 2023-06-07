#include "anydsl_runtime.h"
#include <array>
#include <iostream>
#include <vector>

static std::array<const char*, 4> sDeviceTypes = {
    "Host",
    "Cuda",
    "OpenCL",
    "HSA"
};

int main(int argc, char** argv)
{
    AnyDSLFeatures features = {
        AnyDSL_STRUCTURE_TYPE_FEATURES,
        nullptr
    };
    if (anydslGetFeatures(&features) != AnyDSL_SUCCESS) {
        std::cout << "Could not query features of AnyDSL" << std::endl;
        return -1;
    }

    if (features.bHasJIT != AnyDSL_TRUE) {
        std::cout << "Your AnyDSL runtime does has no JIT support" << std::endl;
        return -2;
    }

    return 0;
}