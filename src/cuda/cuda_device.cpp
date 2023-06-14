#include "cuda_device.h"
#include "cache.h"
#include "cuda_buffer.h"
#include "cuda_event.h"
#include "utils.h"

#include <filesystem>

#ifndef LIBDEVICE_DIR
#define LIBDEVICE_DIR AnyDSL_runtime_LIBDEVICE_DIR
#endif

#ifdef AnyDSL_runtime_HAS_LLVM_SUPPORT
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Linker/Linker.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#endif

namespace AnyDSLInternal {

AnyDSLResult CudaDevice::init()
{
    constexpr int MaxNameLength = 128;
    char name[MaxNameLength];
    CUresult err = CUDA_SUCCESS;

    err = cuDeviceGet(&mDevice, mId);

    CHECK_CUDA_RET(err, "cuDeviceGet()");
    err = cuDeviceGetName(name, MaxNameLength, mDevice);
    CHECK_CUDA_RET(err, "cuDeviceGetName()");
    err = cuDeviceGetAttribute(&mMajorVersion, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR, mDevice);
    CHECK_CUDA_RET(err, "cuDeviceGetAttribute()");
    err = cuDeviceGetAttribute(&mMinorVersion, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MINOR, mDevice);
    CHECK_CUDA_RET(err, "cuDeviceGetAttribute()");

    // Enforce the string to be null-terminated.
    name[MaxNameLength - 1] = 0;
    mName                   = name;

    mComputeCapability = (CUjit_target)(mMajorVersion * 10 + mMinorVersion);
    debug("  (%d) %s, Compute capability: %d.%d", mId, name, mMajorVersion, mMinorVersion);

    err = cuCtxCreate(&mContext, CU_CTX_MAP_HOST, mDevice);
    CHECK_CUDA_RET(err, "cuCtxCreate()");

    return AnyDSL_SUCCESS;
}

AnyDSLResult CudaDevice::get_handle(AnyDSLDeviceHandleInfo* pInfo)
{
    CHECK_RET_PTR(pInfo);
    CHECK_RET_TYPE(pInfo, AnyDSL_STRUCTURE_TYPE_DEVICE_HANDLE_INFO);

    pInfo->pHandle = (void*)((uintptr_t)mContext);

    return AnyDSL_SUCCESS;
}

AnyDSLResult CudaDevice::get_info(AnyDSLDeviceInfo* pInfo)
{
    CHECK_RET_PTR(pInfo);
    CHECK_RET_TYPE(pInfo, AnyDSL_STRUCTURE_TYPE_DEVICE_INFO);
    // TODO: Iterate through the chain?

    size_t bytes = 0;
    CUresult err = cuDeviceTotalMem(&bytes, mDevice);
    CHECK_CUDA_RET(err, "cuDeviceTotalMem()");

    pInfo->isHost       = AnyDSL_FALSE;
    pInfo->deviceNumber = (uint32_t)mId;
    pInfo->deviceType   = AnyDSL_DEVICE_CUDA;
    pInfo->name         = mName.c_str();
    pInfo->totalMemory  = bytes;
    pInfo->version      = (uint32_t)mComputeCapability;

    return AnyDSL_SUCCESS;
}

AnyDSLResult CudaDevice::get_features(AnyDSLDeviceFeatures* pFeatures)
{
    CHECK_RET_PTR(pFeatures);
    CHECK_RET_TYPE(pFeatures, AnyDSL_STRUCTURE_TYPE_DEVICE_FEATURES);

    // Nothing to fill here

    const void* ptr = pFeatures->pNext;
    while (ptr != nullptr) {
        if (checkChainEntry(ptr, AnyDSL_STRUCTURE_TYPE_DEVICE_FEATURES_CUDA)) {
            AnyDSLDeviceFeaturesCuda* pCudaFeatures = (AnyDSLDeviceFeaturesCuda*)ptr;

            CudaContextGuard ctx(this);

            int prop     = -1;
            CUresult err = cuDeviceGetAttribute(&prop, CU_DEVICE_ATTRIBUTE_MAX_THREADS_PER_BLOCK, mDevice);
            CHECK_CUDA_RET(err, "cuDeviceGetAttribute()");
            pCudaFeatures->maxThreadsPerBlock = prop >= 0 ? (size_t)prop : (size_t)0;

            err = cuDeviceGetAttribute(&prop, CU_DEVICE_ATTRIBUTE_MAX_BLOCK_DIM_X, mDevice);
            CHECK_CUDA_RET(err, "cuDeviceGetAttribute()");
            pCudaFeatures->maxBlockDim[0] = prop >= 0 ? (size_t)prop : (size_t)0;

            err = cuDeviceGetAttribute(&prop, CU_DEVICE_ATTRIBUTE_MAX_BLOCK_DIM_Y, mDevice);
            CHECK_CUDA_RET(err, "cuDeviceGetAttribute()");
            pCudaFeatures->maxBlockDim[1] = prop >= 0 ? (size_t)prop : (size_t)0;

            err = cuDeviceGetAttribute(&prop, CU_DEVICE_ATTRIBUTE_MAX_BLOCK_DIM_Z, mDevice);
            CHECK_CUDA_RET(err, "cuDeviceGetAttribute()");
            pCudaFeatures->maxBlockDim[2] = prop >= 0 ? (size_t)prop : (size_t)0;

            err = cuDeviceGetAttribute(&prop, CU_DEVICE_ATTRIBUTE_MAX_GRID_DIM_X, mDevice);
            CHECK_CUDA_RET(err, "cuDeviceGetAttribute()");
            pCudaFeatures->maxGridDim[0] = prop >= 0 ? (size_t)prop : (size_t)0;

            err = cuDeviceGetAttribute(&prop, CU_DEVICE_ATTRIBUTE_MAX_GRID_DIM_Y, mDevice);
            CHECK_CUDA_RET(err, "cuDeviceGetAttribute()");
            pCudaFeatures->maxGridDim[1] = prop >= 0 ? (size_t)prop : (size_t)0;

            err = cuDeviceGetAttribute(&prop, CU_DEVICE_ATTRIBUTE_MAX_GRID_DIM_Z, mDevice);
            CHECK_CUDA_RET(err, "cuDeviceGetAttribute()");
            pCudaFeatures->maxGridDim[2] = prop >= 0 ? (size_t)prop : (size_t)0;

            err = cuDeviceGetAttribute(&prop, CU_DEVICE_ATTRIBUTE_MAX_SHARED_MEMORY_PER_BLOCK, mDevice);
            CHECK_CUDA_RET(err, "cuDeviceGetAttribute()");
            pCudaFeatures->maxSharedMemPerBlock = prop >= 0 ? (size_t)prop : (size_t)0;

            err = cuDeviceGetAttribute(&prop, CU_DEVICE_ATTRIBUTE_MAX_REGISTERS_PER_BLOCK, mDevice);
            CHECK_CUDA_RET(err, "cuDeviceGetAttribute()");
            pCudaFeatures->maxRegistersPerBlock = prop >= 0 ? (size_t)prop : (size_t)0;

            size_t freeMem, totalMem;
            err = cuMemGetInfo(&freeMem, &totalMem);
            CHECK_CUDA_RET(err, "cuMemGetInfo()");

            pCudaFeatures->freeMemory = freeMem;
        }

        ptr = nextChainEntry(ptr);
    }

    return AnyDSL_SUCCESS;
}

AnyDSLResult CudaDevice::set_options(AnyDSLDeviceOptions* pOptions)
{
    CHECK_RET_PTR(pOptions);
    CHECK_RET_TYPE(pOptions, AnyDSL_STRUCTURE_TYPE_DEVICE_OPTIONS);

    // Nothing to get here

    const void* ptr = pOptions->pNext;
    while (ptr != nullptr) {
        if (checkChainEntry(ptr, AnyDSL_STRUCTURE_TYPE_DEVICE_OPTIONS_CUDA)) {
            AnyDSLDeviceOptionsCuda* pCudaOptions = (AnyDSLDeviceOptionsCuda*)ptr;

            std::lock_guard _guard(mLock);
            mUseNVPTX  = pCudaOptions->useNVPTX == AnyDSL_TRUE;
            mDumpCubin = pCudaOptions->dumpCubin == AnyDSL_TRUE;
        }
    }
    return AnyDSL_SUCCESS;
}

AnyDSLResult CudaDevice::sync()
{
    CudaContextGuard ctx(this);

    CUresult err = cuCtxSynchronize();
    CHECK_CUDA_RET(err, "cuCtxSynchronize()");

    return AnyDSL_SUCCESS;
}

std::tuple<AnyDSLResult, Buffer*> CudaDevice::create_buffer(const AnyDSLCreateBufferInfo* pInfo)
{
    CudaBuffer* buffer = new CudaBuffer(this);

    if (buffer == nullptr)
        return { HANDLE_ERROR(AnyDSL_OUT_OF_HOST_MEMORY), nullptr };

    AnyDSLResult res = buffer->create(pInfo);
    return { res, buffer };
}

std::tuple<AnyDSLResult, Event*> CudaDevice::create_event(const AnyDSLCreateEventInfo* pInfo)
{
    CudaEvent* event = new CudaEvent(this);

    if (event == nullptr)
        return { HANDLE_ERROR(AnyDSL_OUT_OF_HOST_MEMORY), nullptr };

    AnyDSLResult res = event->create(pInfo);
    return { res, event };
}

std::tuple<AnyDSLResult, void*> CudaDevice::allocate_memory(size_t size)
{
    CudaContextGuard ctx(this);

    CUdeviceptr mem;
    CUresult err     = cuMemAlloc(&mem, size);
    AnyDSLResult res = CHECK_CUDA(err, "cuMemAlloc()");

    return { res, reinterpret_cast<void*>(mem) };
}

AnyDSLResult CudaDevice::release_memory(void* ptr)
{
    CudaContextGuard ctx(this);
    CUresult err = cuMemFree((CUdeviceptr)ptr);
    CHECK_CUDA_RET(err, "cuMemFree()");
    return AnyDSL_SUCCESS;
}

AnyDSLResult CudaDevice::launch_kernel(const AnyDSLLaunchKernelInfo* pInfo)
{
    // TODO: Add option to gather information about the functions (like reg size etc)
    CHECK_RET_PTR(pInfo);
    CHECK_RET_TYPE(pInfo, AnyDSL_STRUCTURE_TYPE_DEVICE_LAUNCH_KERNEL_INFO);

    CudaContextGuard ctx(this);

    CUfunction func;
    AnyDSLResult res = load_kernel(pInfo->file_name, pInfo->kernel_name, &func);
    if (res != AnyDSL_SUCCESS)
        return res;

    CUresult err = cuLaunchKernel(func,
                                  pInfo->grid[0] / pInfo->block[0],
                                  pInfo->grid[1] / pInfo->block[1],
                                  pInfo->grid[2] / pInfo->block[2],
                                  pInfo->block[0], pInfo->block[1], pInfo->block[2],
                                  0, nullptr, pInfo->kernelParams, nullptr);

    CHECK_CUDA_RET(err, "cuLaunchKernel()");

    return AnyDSL_SUCCESS;
}

void CudaDevice::make_context()
{
    cuCtxPushCurrent(mContext);
}

void CudaDevice::drop_context()
{
    cuCtxPopCurrent(NULL);
}

// TODO: Get this somehow from the runtime!
#if CUDA_VERSION < 9000
static std::string get_libdevice_path(CUjit_target compute_capability)
{
    // select libdevice module according to documentation
    if (compute_capability < 30)
        return std::string(LIBDEVICE_DIR) + "libdevice.compute_20.10.bc";
    else if (compute_capability == 30)
        return std::string(LIBDEVICE_DIR) + "libdevice.compute_30.10.bc";
    else if (compute_capability < 35)
        return std::string(LIBDEVICE_DIR) + "libdevice.compute_20.10.bc";
    else if (compute_capability <= 37)
        return std::string(LIBDEVICE_DIR) + "libdevice.compute_35.10.bc";
    else if (compute_capability < 50)
        return std::string(LIBDEVICE_DIR) + "libdevice.compute_30.10.bc";
    else if (compute_capability <= 53)
        return std::string(LIBDEVICE_DIR) + "libdevice.compute_50.10.bc";
    return std::string(LIBDEVICE_DIR) + "libdevice.compute_30.10.bc";
}
#else
std::string get_libdevice_path(CUjit_target)
{
    return std::string(LIBDEVICE_DIR) + "libdevice.10.bc";
}
#endif

AnyDSLResult CudaDevice::load_kernel(const std::string& filename, const std::string& kernelname, CUfunction* func)
{
    // lock the device when the function cache is accessed
    std::lock_guard<std::mutex> _guard(mLock);

    CUmodule mod;
    auto canonical  = std::filesystem::weakly_canonical(filename);
    auto& mod_cache = mModules;
    auto mod_it     = mod_cache.find(canonical.string());
    if (mod_it == mod_cache.end()) {
        if (canonical.extension() != ".nvvm")
            error("Incorrect extension for kernel file '%s' (should be '.ptx', '.cu', or '.nvvm')", canonical.string().c_str());

        // load file from disk or cache
        auto src_path = canonical;
        // if (src_path.extension() == ".nvvm")
        //     src_path.replace_extension(".nvvm.bc");

        std::string src_code = Cache::instance().load_file(src_path.string());

        // compile src or load from cache
        std::string compute_capability_str = std::to_string(mComputeCapability);
        std::string ptx                    = Cache::instance().load_from_cache(compute_capability_str + src_code);
        if (ptx.empty()) {
#ifdef AnyDSL_runtime_HAS_LLVM_SUPPORT
            if (mUseNVPTX) {
#else
            if (false) {
#endif
                const AnyDSLResult compileRes = compile_nvptx(src_path.string(), src_code, &ptx);
                if (compileRes != AnyDSL_SUCCESS)
                    return compileRes;
            } else {
                const AnyDSLResult compileRes = compile_nvvm(src_path.string(), src_code, &ptx);
                if (compileRes != AnyDSL_SUCCESS)
                    return compileRes;
            }

            Cache::instance().store_to_cache(compute_capability_str + src_code, ptx);
        }

        const AnyDSLResult moduleRes = create_module(src_path.string(), ptx, &mod);
        if (moduleRes != AnyDSL_SUCCESS)
            return moduleRes;

        mod_cache[canonical.string()] = mod;
    } else {
        mod = mod_it->second;
    }

    // checks that the function exists
    auto& func_cache = mFunctions;
    auto& func_map   = func_cache[mod];
    auto func_it     = func_map.find(kernelname);

    if (func_it == func_map.end()) {
        CUresult err = cuModuleGetFunction(func, mod, kernelname.c_str());
        if (err != CUDA_SUCCESS)
            info("Function '%s' is not present in '%s'", kernelname.c_str(), filename.c_str());
        CHECK_CUDA_RET(err, "cuModuleGetFunction()");
        int regs, cmem, lmem, smem, threads;
        err = cuFuncGetAttribute(&regs, CU_FUNC_ATTRIBUTE_NUM_REGS, *func);
        CHECK_CUDA_RET(err, "cuFuncGetAttribute()");
        err = cuFuncGetAttribute(&smem, CU_FUNC_ATTRIBUTE_SHARED_SIZE_BYTES, *func);
        CHECK_CUDA_RET(err, "cuFuncGetAttribute()");
        err = cuFuncGetAttribute(&cmem, CU_FUNC_ATTRIBUTE_CONST_SIZE_BYTES, *func);
        CHECK_CUDA_RET(err, "cuFuncGetAttribute()");
        err = cuFuncGetAttribute(&lmem, CU_FUNC_ATTRIBUTE_LOCAL_SIZE_BYTES, *func);
        CHECK_CUDA_RET(err, "cuFuncGetAttribute()");
        err = cuFuncGetAttribute(&threads, CU_FUNC_ATTRIBUTE_MAX_THREADS_PER_BLOCK, *func);
        CHECK_CUDA_RET(err, "cuFuncGetAttribute()");
        debug("Function '%s' using %i registers, %i | %i | %i bytes shared | constant | local memory allowing up to %i threads per block", kernelname.c_str(), regs, smem, cmem, lmem, threads);

        func_cache[mod][kernelname] = *func;
    } else {
        *func = func_it->second;
    }

    return AnyDSL_SUCCESS;
}

#ifdef AnyDSL_runtime_HAS_LLVM_SUPPORT
AnyDSLResult CudaDevice::compile_nvptx(const std::string& filename, const std::string& program_string, std::string* compiled) const
{
    static bool llvm_nvptx_initialized = false;
    if (!llvm_nvptx_initialized) {
        // TODO: Make this an option
        // ANYDSL_LLVM_ARGS="--nvptx-sched4reg --nvptx-fma-level=2 --nvptx-prec-divf32=0 --nvptx-prec-sqrtf32=0 --nvptx-f32ftz=1"
        const char* env_var = std::getenv("ANYDSL_LLVM_ARGS");

        std::vector<std::string> llvm_args = { "nvptx" };
        if (env_var) {
            std::istringstream stream(env_var);
            std::string tmp;
            while (stream >> tmp)
                llvm_args.push_back(tmp);
        } else {
            llvm_args.emplace_back("--nvptx-sched4reg");
            llvm_args.emplace_back("--nvptx-fma-level=2");
            llvm_args.emplace_back("--nvptx-prec-divf32=0");
            llvm_args.emplace_back("--nvptx-prec-sqrtf32=0");
            // llvm_args.emplace_back("--nvptx-f32ftz=1");
        }

        if (llvm_args.size() > 1) {
            std::vector<const char*> c_llvm_args;
            for (auto& str : llvm_args)
                c_llvm_args.push_back(str.c_str());
            llvm::cl::ParseCommandLineOptions(c_llvm_args.size(), c_llvm_args.data(), "AnyDSL nvptx JIT compiler\n");
        }

        LLVMInitializeNVPTXTarget();
        LLVMInitializeNVPTXTargetInfo();
        LLVMInitializeNVPTXTargetMC();
        LLVMInitializeNVPTXAsmPrinter();
        llvm_nvptx_initialized = true;
    }

    const std::string cpu = "sm_" + std::to_string(mComputeCapability);

    llvm::LLVMContext llvm_context;
    llvm::SMDiagnostic diagnostic_err;
    std::unique_ptr<llvm::Module> llvm_module = llvm::parseIR(llvm::MemoryBuffer::getMemBuffer(program_string)->getMemBufferRef(), diagnostic_err, llvm_context);

    if (!llvm_module) {
        std::string stream;
        llvm::raw_string_ostream llvm_stream(stream);
        diagnostic_err.print("", llvm_stream);
        error("Parsing IR file %s: %s", filename, llvm_stream.str());

        return HANDLE_ERROR(AnyDSL_PLATFORM_ERROR);
    }

    auto triple_str = llvm_module->getTargetTriple();
    std::string error_str;
    auto target = llvm::TargetRegistry::lookupTarget(triple_str, error_str);
    llvm::TargetOptions options;
    options.AllowFPOpFusion = llvm::FPOpFusion::Fast;
    std::unique_ptr<llvm::TargetMachine> machine(target->createTargetMachine(triple_str, cpu, "" /* attrs */, options, llvm::Reloc::PIC_, llvm::CodeModel::Small, llvm::CodeGenOpt::Aggressive));

    // link libdevice
    std::string libdevice_filename = get_libdevice_path(mComputeCapability);
    std::unique_ptr<llvm::Module> libdevice_module(llvm::parseIRFile(libdevice_filename, diagnostic_err, llvm_context));
    if (libdevice_module == nullptr) {
        error("Can't create libdevice module for '%s'", libdevice_filename);
        return HANDLE_ERROR(AnyDSL_PLATFORM_ERROR);
    }

    // override data layout with the one coming from the target machine
    llvm_module->setDataLayout(machine->createDataLayout());
    libdevice_module->setDataLayout(machine->createDataLayout());
    libdevice_module->setTargetTriple(triple_str);

    llvm::Linker linker(*llvm_module.get());
    if (linker.linkInModule(std::move(libdevice_module), llvm::Linker::Flags::LinkOnlyNeeded)) {
        error("Can't link libdevice into module");
        return HANDLE_ERROR(AnyDSL_PLATFORM_ERROR);
    }

    llvm::legacy::FunctionPassManager function_pass_manager(llvm_module.get());
    llvm::legacy::PassManager module_pass_manager;

    module_pass_manager.add(llvm::createTargetTransformInfoWrapperPass(machine->getTargetIRAnalysis()));
    function_pass_manager.add(llvm::createTargetTransformInfoWrapperPass(machine->getTargetIRAnalysis()));

    llvm::PassManagerBuilder builder;
    builder.OptLevel = 3; // Full opt
    builder.Inliner  = llvm::createFunctionInliningPass(builder.OptLevel, 0, false);
    machine->adjustPassManager(builder);
    builder.populateFunctionPassManager(function_pass_manager);
    builder.populateModulePassManager(module_pass_manager);

    machine->Options.MCOptions.AsmVerbose = true;

    llvm::SmallString<0> outstr;
    llvm::raw_svector_ostream llvm_stream(outstr);

    machine->addPassesToEmitFile(module_pass_manager, llvm_stream, nullptr, llvm::CodeGenFileType::CGFT_AssemblyFile, true);

    function_pass_manager.doInitialization();
    for (auto func = llvm_module->begin(); func != llvm_module->end(); ++func)
        function_pass_manager.run(*func);
    function_pass_manager.doFinalization();
    module_pass_manager.run(*llvm_module);

    *compiled = outstr.str().str();
    return AnyDSL_SUCCESS;
}
#else
AnyDSLResult CudaDevice::compile_nvptx(const std::string&, const std::string&, std::string*) const
{
    return HANDLE_ERROR(AnyDSL_NOT_SUPPORTED);
}
#endif

#if CUDA_VERSION < 10000
#define nvvmLazyAddModuleToProgram(prog, buffer, size, name) nvvmAddModuleToProgram(prog, buffer, size, name)
#endif
AnyDSLResult CudaDevice::compile_nvvm(const std::string& filename, const std::string& program_string, std::string* compiled) const
{
    nvvmProgram program;
    nvvmResult err = nvvmCreateProgram(&program);
    CHECK_NVVM_RET(err, "nvvmCreateProgram()");

    std::string libdevice_filename = get_libdevice_path(mComputeCapability);
    std::string libdevice_string   = Cache::instance().load_file(libdevice_filename);
    err                            = nvvmLazyAddModuleToProgram(program, libdevice_string.c_str(), libdevice_string.length(), libdevice_filename.c_str());
    CHECK_NVVM_RET(err, "nvvmAddModuleToProgram()");

    err = nvvmAddModuleToProgram(program, program_string.c_str(), program_string.length(), filename.c_str());
    CHECK_NVVM_RET(err, "nvvmAddModuleToProgram()");

    std::string compute_arch("-arch=compute_" + std::to_string(mComputeCapability));
    int num_options       = 7;
    const char* options[] = {
        // TODO: Convert these options
        compute_arch.c_str(),
        "-opt=3",
        "-generate-line-info",
        "-ftz=1",
        "-prec-div=0",
        "-prec-sqrt=0",
        "-fma=1",
        //   "-g"
    };

    err = nvvmVerifyProgram(program, num_options, options);
    if (err != NVVM_SUCCESS) {
        size_t log_size;
        nvvmGetProgramLogSize(program, &log_size);
        std::string error_log(log_size, '\0');
        nvvmGetProgramLog(program, &error_log[0]);
        error("Verify program error: %s", error_log.c_str());

        Cache::instance().store_file(filename + ".sm_" + std::to_string(mComputeCapability) + ".err.nvvm", program_string);

        CHECK_NVVM_RET(err, "nvvmVerifyProgram");
    }

    debug("Compiling NVVM to PTX using libNVVM for '%s' on CUDA device %i", filename.c_str(), mId);
    err = nvvmCompileProgram(program, num_options, options);
    if (err != NVVM_SUCCESS) {
        size_t log_size;
        nvvmGetProgramLogSize(program, &log_size);
        std::string error_log(log_size, '\0');
        nvvmGetProgramLog(program, &error_log[0]);
        error("Compilation error: %s", error_log.c_str());

        Cache::instance().store_file(filename + ".sm_" + std::to_string(mComputeCapability) + ".err.nvvm", program_string);

        CHECK_NVVM_RET(err, "nvvmCompileProgram");
    }
    CHECK_NVVM_RET(err, "nvvmCompileProgram()");

    size_t ptx_size;
    err = nvvmGetCompiledResultSize(program, &ptx_size);
    CHECK_NVVM_RET(err, "nvvmGetCompiledResultSize()");

    std::string ptx(ptx_size, '\0');
    err = nvvmGetCompiledResult(program, &ptx[0]);
    CHECK_NVVM_RET(err, "nvvmGetCompiledResult()");

    err = nvvmDestroyProgram(&program);
    CHECK_NVVM_RET(err, "nvvmDestroyProgram()");

    *compiled = std::move(ptx);

    return AnyDSL_SUCCESS;
}

AnyDSLResult CudaDevice::create_module(const std::string& filename, const std::string& ptx_string, CUmodule* module) const
{
    // TODO: Expose optimization level
    constexpr unsigned int opt_level = 4;

    debug("Creating module from PTX '%s' on CUDA device %i", filename.c_str(), mId);

    char info_log[10240]  = {};
    char error_log[10240] = {};

    CUjit_option options[] = {
        CU_JIT_INFO_LOG_BUFFER, CU_JIT_INFO_LOG_BUFFER_SIZE_BYTES,
        CU_JIT_ERROR_LOG_BUFFER, CU_JIT_ERROR_LOG_BUFFER_SIZE_BYTES,
        CU_JIT_TARGET,
        CU_JIT_OPTIMIZATION_LEVEL
    };

    void* option_values[] = {
        info_log, reinterpret_cast<void*>(static_cast<uintptr_t>(std::size(info_log))),
        error_log, reinterpret_cast<void*>(static_cast<uintptr_t>(std::size(error_log))),
        reinterpret_cast<void*>(static_cast<uintptr_t>(mComputeCapability)),
        reinterpret_cast<void*>(static_cast<uintptr_t>(opt_level))
    };

    static_assert(std::size(options) == std::size(option_values));

    CUlinkState linker;
    CHECK_CUDA_RET(cuLinkCreate(std::size(options), options, option_values, &linker), "cuLinkCreate()");

    CHECK_CUDA_RET(cuLinkAddData(linker, CU_JIT_INPUT_PTX, const_cast<char*>(ptx_string.c_str()), ptx_string.length(), filename.c_str(), 0U, nullptr, nullptr), "cuLinkAddData");

    void* binary;
    size_t binary_size;
    CUresult err = cuLinkComplete(linker, &binary, &binary_size);
    if (err != CUDA_SUCCESS)
        error("Compilation error: %s", error_log);
    if (*info_log)
        info("Compilation info: %s", info_log);
    CHECK_CUDA_RET(err, "cuLinkComplete()");

    if (mDumpCubin) {
        auto cubin_name = filename + ".sm_" + std::to_string(mComputeCapability) + ".cubin";
        Cache::instance().store_file(cubin_name, static_cast<const std::byte*>(binary), binary_size);
    }

    CHECK_CUDA_RET(cuModuleLoadData(module, binary), "cuModuleLoadData()");

    CHECK_CUDA_RET(cuLinkDestroy(linker), "cuLinkDestroy");

    return AnyDSL_SUCCESS;
}
} // namespace AnyDSLInternal