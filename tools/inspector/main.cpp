#include "anydsl_runtime.h"
#include "anydsl_runtime.hpp"
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
    size_t count = 0;
    if (anydslEnumerateDevices(&count, nullptr) != AnyDSL_SUCCESS) {
        std::cout << "Could not enumerate devices." << std::endl;
        return -1;
    }

    if (count == 0) {
        std::cout << "No AnyDSL supported devices found." << std::endl;
        return 0;
    }

    std::cout << "Found " << count << " devices" << std::endl;

    std::vector<AnyDSLDeviceInfo> infos(count);
    for (size_t i = 0; i < count; ++i) {
        infos[i].sType = AnyDSL_STRUCTURE_TYPE_DEVICE_INFO;
        infos[i].pNext = nullptr;
    }

    if (anydslEnumerateDevices(&count, infos.data()) != AnyDSL_SUCCESS) {
        std::cout << "Could not enumerate devices." << std::endl;
        return -1;
    }

    for (size_t i = 0; i < count; ++i) {
        std::cout << "(" << i << ") => " << infos[i].name << std::endl
                  << " - Type     : " << sDeviceTypes.at(infos[i].deviceType) << std::endl
                  << " - Number   : " << infos[i].deviceNumber << std::endl
                  << " - Version  : " << infos[i].version << std::endl
                  << " - Total MB : " << (infos[i].totalMemory / (1024 * 1024)) << std::endl
                  << " - Is Host  : " << (infos[i].isHost != AnyDSL_FALSE ? "Yes" : "No") << std::endl;

        AnyDSLGetDeviceRequest req = {
            AnyDSL_STRUCTURE_TYPE_GET_DEVICE_REQUEST,
            nullptr,
            infos[i].deviceType,
            infos[i].deviceNumber
        };
        AnyDSLDevice device;
        if (anydslGetDevice(&req, &device) != AnyDSL_SUCCESS) {
            std::cout << "Could not get device " << i << "." << std::endl;
            return -1;
        }

        if (infos[i].deviceType == AnyDSL_DEVICE_CUDA) {
            AnyDSLDeviceFeaturesCuda cudaFeatures = {
                AnyDSL_STRUCTURE_TYPE_DEVICE_FEATURES_CUDA,
                nullptr
                // Will be initialized later
            };

            AnyDSLDeviceFeatures features = {
                AnyDSL_STRUCTURE_TYPE_DEVICE_FEATURES,
                &cudaFeatures
                // Will be initialized later
            };

            if (anydslGetDeviceFeatures(device, &features) != AnyDSL_SUCCESS) {
                std::cout << "Could not get device " << i << " features." << std::endl;
                return -1;
            }

            std::cout << " - Features >" << std::endl
                      << "   - Max Block                : [" << cudaFeatures.maxBlockDim[0] << ", " << cudaFeatures.maxBlockDim[1] << ", " << cudaFeatures.maxBlockDim[2] << "]" << std::endl
                      << "   - Max Grid                 : [" << cudaFeatures.maxGridDim[0] << ", " << cudaFeatures.maxGridDim[1] << ", " << cudaFeatures.maxGridDim[2] << "]" << std::endl
                      << "   - Max Reg Per Block        : " << cudaFeatures.maxRegistersPerBlock << std::endl
                      << "   - Max Shared Mem Per Block : " << cudaFeatures.maxSharedMemPerBlock << "b" << std::endl
                      << "   - Free MB                  : " << (cudaFeatures.freeMemory / (1024 * 1024)) << std::endl;
        }
    }

    return 0;
}