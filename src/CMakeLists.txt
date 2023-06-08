# Necessary to specify whether or not to build shared libraries on a per-project basis
option(AnyDSL_runtime_BUILD_SHARED "Builds a shared library for the runtime (and JIT, if enabled)" ${BUILD_SHARED_LIBS})

if(AnyDSL_runtime_BUILD_SHARED)
    set(AnyDSL_runtime_BUILD "SHARED")
else()
    set(AnyDSL_runtime_BUILD "STATIC")
endif()

set(AnyDSL_runtime_CONFIG_FILE ${CMAKE_BINARY_DIR}/include/anydsl_runtime_config.h)
set(AnyDSL_runtime_INTERNAL_CONFIG_FILE ${CMAKE_BINARY_DIR}/include/anydsl_runtime_internal_config.h)

set(SRC
    ${AnyDSL_runtime_CONFIG_FILE}
    ${AnyDSL_runtime_INTERNAL_CONFIG_FILE}
    anydsl_runtime.cpp
    anydsl_runtime_compatibility.cpp
    anydsl_runtime_std.cpp
    cache.cpp
    cache.h
    device.h
    dummy_platform.h
    event.h
    log.cpp
    log.h
    platform.h
    runtime.cpp
    runtime.h
    utils.cpp
    utils.h
    host/cpu_buffer.cpp
    host/cpu_buffer.h
    host/cpu_device.cpp
    host/cpu_device.h
    host/cpu_event.cpp
    host/cpu_event.h
    host/cpu_platform.cpp
    host/cpu_platform.h
    )

add_library(runtime ${AnyDSL_runtime_BUILD} ${SRC})
target_include_directories(runtime
    PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>)

if(MSVC)
    target_compile_options(runtime PRIVATE "/experimental:external" "/external:anglebrackets" "/external:W0")
else()
    target_compile_options(runtime PRIVATE "-Wall" "-Wextra")
endif()

# look for CUDA
set(CUDA_SRC
    cuda/cuda_buffer.cpp
    cuda/cuda_buffer.h
    cuda/cuda_device.cpp
    cuda/cuda_device.h
    cuda/cuda_event.cpp
    cuda/cuda_event.h
    cuda/cuda_inc.h
    cuda/cuda_platform.cpp
    cuda/cuda_platform.h)

if(${CMAKE_VERSION} VERSION_LESS "3.18.0")
    find_package(CUDA QUIET)

    if(CUDA_FOUND)
        find_library(CUDA_NVVM_LIBRARY nvvm
            HINTS ${CUDA_TOOLKIT_ROOT_DIR}/nvvm
            PATH_SUFFIXES lib lib64 lib/x64)
        find_library(CUDA_NVRTC_LIBRARY nvrtc
            HINTS ${CUDA_TOOLKIT_ROOT_DIR}
            PATH_SUFFIXES lib lib64 lib/x64)

        if(CUDA_NVRTC_LIBRARY)
            set(AnyDSL_runtime_CUDA_NVRTC TRUE)
        else()
            set(CUDA_NVRTC_LIBRARY "")
        endif()

        find_library(CUDA_LIBRARY cuda
            HINTS ${CUDA_TOOLKIT_ROOT_DIR}
            PATH_SUFFIXES lib lib64 lib/x64)
        target_sources(runtime PRIVATE ${CUDA_SRC})
        target_include_directories(runtime PRIVATE ${CUDA_INCLUDE_DIRS} "${CUDA_TOOLKIT_ROOT_DIR}/nvvm/include")
        target_link_libraries(runtime PRIVATE ${CUDA_LIBRARY} ${CUDA_NVVM_LIBRARY} ${CUDA_NVRTC_LIBRARY})

        # TODO: would be nice to reference directly the file
        find_path(AnyDSL_runtime_LIBDEVICE_DIR
            NAMES libdevice.10.bc libdevice.compute_50.10.bc libdevice.compute_35.10.bc libdevice.compute_30.10.bc libdevice.compute_20.10.bc
            HINTS ${CUDA_TOOLKIT_ROOT_DIR}
            PATH_SUFFIXES nvvm/libdevice)
        mark_as_advanced(AnyDSL_runtime_LIBDEVICE_DIR)
    endif()

    set(AnyDSL_runtime_HAS_CUDA_SUPPORT ${CUDA_FOUND} CACHE INTERNAL "enables CUDA/NVVM support")
else()
    find_package(CUDAToolkit QUIET)

    if(CUDAToolkit_FOUND)
        # this is more robust in case CUDAToolkit_LIBRARY_ROOT is not defined
        find_path(CUDAToolkit_LIBRARY_ROOT
            NAMES version.txt version.json bin/nvcc
            HINTS ${CUDAToolkit_LIBRARY_ROOT} ${CUDAToolkit_ROOT} ${CUDAToolkit_BIN_DIR}/../)
        find_library(CUDAToolkit_NVVM_LIBRARY nvvm
            HINTS ${CUDAToolkit_LIBRARY_ROOT}/nvvm
            PATH_SUFFIXES lib lib64 lib/x64)
        target_sources(runtime PRIVATE ${CUDA_SRC})
        target_include_directories(runtime PRIVATE "${CUDAToolkit_LIBRARY_ROOT}/nvvm/include")
        target_link_libraries(runtime PRIVATE CUDA::cuda_driver CUDA::nvrtc ${CUDAToolkit_NVVM_LIBRARY})

        # TODO: would be nice to reference directly the file
        find_path(AnyDSL_runtime_LIBDEVICE_DIR
            NAMES libdevice.10.bc
            HINTS ${CUDAToolkit_LIBRARY_ROOT} ${CUDAToolkit_LIBRARY_DIR}
            PATH_SUFFIXES cuda/nvvm/libdevice nvvm/libdevice)

        mark_as_advanced(AnyDSL_runtime_LIBDEVICE_DIR AnyDSL_runtime_NVCC_BIN AnyDSL_runtime_NVCC_INC)
    endif()

    set(AnyDSL_runtime_HAS_CUDA_SUPPORT ${CUDAToolkit_FOUND} CACHE INTERNAL "enables CUDA/NVVM support")
endif()

if(AnyDSL_runtime_HAS_CUDA_SUPPORT)
    set(AnyDSL_runtime_CUDA_CXX_STANDARD 11 CACHE STRING "C++ version to use for CUDA code")
endif()

# look for OpenCL
# find_package(OpenCL)
if(OpenCL_FOUND)
    target_sources(runtime PRIVATE opencl_platform.cpp opencl_platform.h)
    target_link_libraries(runtime PRIVATE OpenCL::OpenCL)

    # look for Xilinx-HLS
    find_package(XHLS)

    # if(XHLS_FOUND)
    # target_include_directories(runtime PRIVATE ${Xilinx_INCLUDE_DIRS})
    # endif()
endif()

set(AnyDSL_runtime_HAS_OPENCL_SUPPORT ${OpenCL_FOUND} CACHE INTERNAL "enables OpenCL support")

# look for HSA
# find_package(hsa-runtime64 PATHS /opt/rocm)
if(hsa-runtime64_FOUND)
    target_sources(runtime PRIVATE hsa_platform.cpp hsa_platform.h)
    target_link_libraries(runtime PRIVATE hsa-runtime64::hsa-runtime64)

    find_package(AMDDeviceLibs PATHS /opt/rocm)
    get_target_property(ocml_LOCATION ocml LOCATION)
    get_filename_component(AnyDSL_runtime_HSA_BITCODE_PATH ${ocml_LOCATION} DIRECTORY)
    get_filename_component(AnyDSL_runtime_HSA_BITCODE_SUFFIX ${ocml_LOCATION} EXT)
endif()

set(AnyDSL_runtime_HAS_HSA_SUPPORT ${hsa-runtime64_FOUND} CACHE INTERNAL "enables HSA support")

# look for LLVM for nvptx and gcn
find_package(LLVM)

if(LLVM_FOUND)
    target_compile_definitions(runtime PRIVATE ${LLVM_DEFINITIONS})
    target_include_directories(runtime SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})
    set(AnyDSL_runtime_LLVM_COMPONENTS irreader support ${LLVM_TARGETS_TO_BUILD})
    set(AnyDSL_runtime_JIT_LLVM_COMPONENTS ${AnyDSL_runtime_LLVM_COMPONENTS} mcjit)

    if(AnyDSL_runtime_HAS_HSA_SUPPORT)
        find_package(LLD REQUIRED)
        target_link_libraries(runtime PRIVATE lldELF lldCommon)
        llvm_config(runtime ${AnyDSL_LLVM_LINK_SHARED} lto option ${LLVM_TARGETS_TO_BUILD})
    endif()
endif()

set(AnyDSL_runtime_HAS_LLVM_SUPPORT ${LLVM_FOUND} CACHE INTERNAL "enables nvptx / gcn support")

if(AnyDSL_RUNTIME_JIT)
    function(add_runtime_jit frontend)
        find_package(Python3 COMPONENTS Interpreter REQUIRED)
        if("${frontend}" STREQUAL "artic")
            file(GLOB RUNTIME_FRONTEND_SRCS ../platforms/${frontend}/*.art)
        else()
            file(GLOB RUNTIME_FRONTEND_SRCS ../platforms/${frontend}/*.impala)
        endif()

        set(RUNTIME_JIT_SRC jit.cpp jit.h)
        set(RUNTIME_SOURCES_FRONTEND_INC_FILE ${CMAKE_CURRENT_BINARY_DIR}/${frontend}/runtime_srcs.inc)
        add_custom_command(OUTPUT ${RUNTIME_SOURCES_FRONTEND_INC_FILE}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/${frontend}
            COMMAND ${Python3_EXECUTABLE} extract_runtime_srcs.py ${RUNTIME_FRONTEND_SRCS} > ${RUNTIME_SOURCES_FRONTEND_INC_FILE}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            DEPENDS extract_runtime_srcs.py ${RUNTIME_FRONTEND_SRCS})
        target_sources(runtime PRIVATE ${RUNTIME_JIT_SRC} ${RUNTIME_SOURCES_FRONTEND_INC_FILE})
        target_include_directories(runtime PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/${frontend} ${Thorin_INCLUDE_DIRS})
        set_target_properties(runtime PROPERTIES CXX_VISIBILITY_PRESET hidden)
        set_source_files_properties(${RUNTIME_SOURCES_FRONTEND_INC_FILE} PROPERTIES GENERATED TRUE)
        target_compile_definitions(runtime PRIVATE "ANYDSL_ENABLE_FRONTEND_${frontend}")
    endfunction()

    find_package(Artic QUIET)
    find_package(Impala QUIET)

    if(NOT(Artic_FOUND OR Impala_FOUND))
        message(FATAL_ERROR "Enabling AnyDSL_RUNTIME_JIT requires artic or impala.\nPlease specify either Artic_DIR or Impala_DIR or both.")
    endif()

    find_package(Thorin REQUIRED)

    if(NOT Thorin_HAS_LLVM_SUPPORT)
        message(FATAL_ERROR "Enabling AnyDSL_RUNTIME_JIT requires Thorin to be built with LLVM")
    endif()

    if(NOT LLVM_FOUND)
        message(FATAL_ERROR "Enabling AnyDSL_RUNTIME_JIT requires runtime to be built with LLVM")
    endif()

    if(Artic_FOUND)
        message(STATUS "Found Artic: ${Artic_DIR}")
        add_runtime_jit(artic)
        target_link_libraries(runtime PRIVATE ${Thorin_LIBRARIES} ${Artic_LIBRARY})
        set(AnyDSL_runtime_HAS_ARTIC_LANGUAGE ${Artic_FOUND} CACHE INTERNAL "Enables artic language support for JIT")
    endif()

    # Currently we only support one language
    # if(Impala_FOUND)
    #     message(STATUS "Found Impala: ${Impala_DIR}")
    #     add_runtime_jit(impala)
    #     target_link_libraries(runtime PRIVATE ${Thorin_LIBRARIES} ${Impala_LIBRARY})
    #     set(AnyDSL_runtime_HAS_IMPALA_LANGUAGE ${Impala_FOUND} CACHE INTERNAL "Enables impala language support for JIT")
    # endif()
    set(HAS_JIT ON)
else()
    target_sources(runtime PRIVATE jit_null.cpp)
    set(HAS_JIT OFF)
endif()

set(AnyDSL_runtime_HAS_JIT_SUPPORT ${HAS_JIT} CACHE INTERNAL "Enables anydsl_compile() API")

target_include_directories(runtime PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>)
configure_file(anydsl_runtime_internal_config.h.in ${AnyDSL_runtime_INTERNAL_CONFIG_FILE} @ONLY)
set_source_files_properties(${AnyDSL_runtime_CONFIG_FILE} PROPERTIES GENERATED TRUE)

# System threads are required to use either TBB or C++11 threads
find_package(Threads REQUIRED)
target_link_libraries(runtime PRIVATE Threads::Threads)

# TBB is optional, C++11 threads are used when it is not available
find_package(TBB QUIET)

if(TBB_FOUND)
    message(STATUS "Found TBB in ${TBB_DIR}")
    target_link_libraries(runtime PRIVATE TBB::tbb)
endif()

set(AnyDSL_runtime_HAS_TBB_SUPPORT ${TBB_FOUND} CACHE INTERNAL "enables parallel using TBB")

set_target_properties(runtime PROPERTIES DEFINE_SYMBOL "AnyDSL_runtime_EXPORTS")
set_target_properties(runtime PROPERTIES CXX_VISIBILITY_PRESET hidden)

if(LLVM_FOUND)
    llvm_config(runtime ${AnyDSL_LLVM_LINK_SHARED} ${AnyDSL_runtime_LLVM_COMPONENTS})
endif()

set(RUNTIME_LIBRARIES runtime PARENT_SCOPE)
