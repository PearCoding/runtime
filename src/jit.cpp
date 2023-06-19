#include <fstream>
#include <memory>
#include <sstream>

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/ExecutionEngine/RuntimeDyld.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_os_ostream.h>

#include <thorin/be/codegen.h>
#include <thorin/be/llvm/cpu.h>
#include <thorin/world.h>

#include "cache.h"
#include "jit.h"
#include "log.h"
#include "runtime.h"
#include "utils.h"

// NOTE: This file is only compiled if jit is turned ON

bool compile(
    const std::vector<std::string>& file_names,
    const std::vector<std::string>& file_data,
    thorin::World& world,
    std::ostream& error_stream);

namespace AnyDSLInternal {
static const char runtime_srcs[] = {
#include "runtime_srcs.inc"
    0
};

struct CacheDirPushup {
    std::filesystem::path original;

    inline CacheDirPushup(const char* cacheDir)
    {
        original = Cache::instance().get_user_directory();
        if (cacheDir != nullptr)
            Cache::instance().set_directory(cacheDir);
    }

    inline ~CacheDirPushup()
    {
        Cache::instance().set_directory(original);
    }
};

struct JITSingleton {
    struct Program {
        Program(llvm::ExecutionEngine* engine)
            : engine(engine)
        {
        }
        llvm::ExecutionEngine* engine;
    };

    std::vector<Program> mPrograms;

    JITSingleton()
    {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();
    }

    size_t compile(const char* program_src, uint32_t size, const AnyDSLJITCompileOptions* pOptions, AnyDSLJITCompileResult* pResult)
    {
        uint32_t opt               = pOptions->optLevel;
        thorin::LogLevel log_level = pOptions->logLevel <= 4 ? static_cast<thorin::LogLevel>(pOptions->logLevel) : thorin::LogLevel::Warn;

        // TODO: bUseCache
        CacheDirPushup cacheSet(pOptions->cacheDir);

        // The LLVM context and module have to be alive for the duration of this function
        std::unique_ptr<llvm::LLVMContext> llvm_context;
        std::unique_ptr<llvm::Module> llvm_module;

        size_t prog_key = std::hash<std::string>{}(program_src);
        std::stringstream hex_stream;
        hex_stream << std::hex << prog_key;
        std::string program_str = std::string(program_src, size);
        std::string cached_llvm = Cache::instance().load_from_cache(program_str, ".llvm");
        std::string module_name = "jit_" + hex_stream.str();
        if (cached_llvm.empty()) {
            bool debug = false;
            assert(pOptions->optLevel <= 3);

            std::stringstream err_stream;
            thorin::World world(module_name);
            world.set(log_level);
            world.set(std::make_shared<thorin::Stream>(err_stream));

            bool good = true;
            if (!::compile(
                    { "runtime", module_name },
                    { std::string(runtime_srcs), program_str },
                    world, err_stream)) {
                error("JIT: error while compiling sources");
                good = false;
            } else {
                world.opt();

                std::string host_triple, host_cpu, host_attr, hls_flags;
                thorin::DeviceBackends backends(world, opt, debug, hls_flags);

                thorin::llvm::CPUCodeGen cg(world, opt, debug, host_triple, host_cpu, host_attr);
                std::tie(llvm_context, llvm_module) = cg.emit_module();

                {
                    std::stringstream stream;
                    llvm::raw_os_ostream llvm_stream(stream);
                    llvm_module->print(llvm_stream, nullptr);
                    Cache::instance().store_to_cache(program_str, stream.str(), ".llvm");
                }

                if (backends.cgs[thorin::DeviceBackends::HLS])
                    error("JIT compilation of hls not supported!");
                for (auto& cg : backends.cgs) {
                    if (cg) {
                        std::ostringstream stream;
                        cg->emit_stream(stream);
                        Cache::instance().store_to_cache(cg->file_ext() + program_str, stream.str(), cg->file_ext());
                        Cache::instance().register_file(module_name + cg->file_ext(), stream.str());
                    }
                }
            }

            if (pResult != nullptr) {
                std::string err = err_stream.str();
                if (!err.empty()) {
                    pResult->logOutput = new char[err.size() + 1];
                    std::memcpy(pResult->logOutput, err.data(), err.size() * sizeof(char));
                    pResult->logOutput[err.size()] = '\0';
                } else {
                    pResult->logOutput = nullptr;
                }
            }

            if (!good)
                return -1;
        } else {
            llvm::SMDiagnostic diagnostic_err;
            llvm_context = std::make_unique<llvm::LLVMContext>();
            llvm_module  = llvm::parseIR(llvm::MemoryBuffer::getMemBuffer(cached_llvm)->getMemBufferRef(), diagnostic_err, *llvm_context);

            if (!llvm_module && pResult != nullptr) {
                std::string tmp;
                auto stream = llvm::raw_string_ostream(tmp);
                diagnostic_err.print(module_name.c_str(), stream, false, true);

                if (!tmp.empty()) {
                    pResult->logOutput = new char[tmp.size() + 1];
                    std::memcpy(pResult->logOutput, tmp.data(), tmp.size() * sizeof(char));
                    pResult->logOutput[tmp.size()] = '\0';
                } else {
                    pResult->logOutput = nullptr;
                }
            }

            if (llvm_module == nullptr)
                return -1;

            auto load_backend_src = [&](std::string ext) {
                std::string cached_src = Cache::instance().load_from_cache(ext + program_str, ext);
                if (!cached_src.empty())
                    Cache::instance().register_file(module_name + ext, cached_src);
            };
            load_backend_src(".cl");
            load_backend_src(".cu");
            load_backend_src(".nvvm");
            load_backend_src(".amdgpu");
        }

        llvm::TargetOptions target_options;
        target_options.AllowFPOpFusion = llvm::FPOpFusion::Fast;

        auto engine = llvm::EngineBuilder(std::move(llvm_module))
                          .setEngineKind(llvm::EngineKind::JIT)
                          .setMCPU(llvm::sys::getHostCPUName())
                          .setTargetOptions(target_options)
                          .setOptLevel(opt == 0 ? llvm::CodeGenOpt::None : opt == 1 ? llvm::CodeGenOpt::Less
                                                                       : opt == 2   ? llvm::CodeGenOpt::Default
                                                                                    :
                                                                                  /* opt == 3 */ llvm::CodeGenOpt::Aggressive)
                          .create();
        if (!engine)
            return -1;

        engine->finalizeObject();
        mPrograms.push_back(Program(engine));

        return mPrograms.size() - 1;
    }

    void* lookup_function(size_t key, const char* fn_name)
    {
        if (key >= mPrograms.size())
            return nullptr;

        return (void*)mPrograms[key].engine->getFunctionAddress(fn_name);
    }

    void link(const char* lib)
    {
        // TODO: Would be nice to have it per module. But I guess LLVM does not allow it.
        llvm::sys::DynamicLibrary::LoadLibraryPermanently(lib);
    }

    bool check_key(size_t key) const { return key < mPrograms.size(); }
};

JITSingleton& jit()
{
    static JITSingleton jit;
    return jit;
}

static inline size_t unwrapModule(AnyDSLJITModule module) { return (size_t)(uintptr_t)module; }

AnyDSLResult JIT::compile(const char* program, size_t size, AnyDSLJITModule* pModule, const AnyDSLJITCompileOptions* pOptions, AnyDSLJITCompileResult* pResult)
{
    if (size == 0)
        return HANDLE_ERROR(AnyDSL_INVALID_VALUE);
    CHECK_RET_PTR(program);
    CHECK_RET_PTR(pModule);

    CHECK_RET_TYPE(pOptions, AnyDSL_STRUCTURE_TYPE_JIT_COMPILE_OPTIONS);
    if (pResult != nullptr)
        CHECK_RET_TYPE(pResult, AnyDSL_STRUCTURE_TYPE_JIT_COMPILE_RESULT);

    if (pOptions->optLevel > 3) // Only allow proper defined levels
        return HANDLE_ERROR(AnyDSL_INVALID_VALUE);
    if (pOptions->logLevel > 4) // Only allow proper defined levels
        return HANDLE_ERROR(AnyDSL_INVALID_VALUE);

    size_t key = jit().compile(program, size, pOptions, pResult);

    *pModule = (AnyDSLJITModule)key;

    return jit().check_key(key) ? AnyDSL_SUCCESS : HANDLE_ERROR(AnyDSL_JIT_ERROR);
}

AnyDSLResult JIT::destroyModule(AnyDSLJITModule module)
{
    if (!jit().check_key(unwrapModule(module)))
        return HANDLE_ERROR(AnyDSL_INVALID_HANDLE);

    // TODO: Make use of it
    return AnyDSL_SUCCESS;
}

AnyDSLResult JIT::freeCompileResult(const AnyDSLJITCompileResult* pResult)
{
    if (pResult == nullptr)
        return AnyDSL_SUCCESS;

    CHECK_RET_TYPE(pResult, AnyDSL_STRUCTURE_TYPE_JIT_COMPILE_RESULT);

    if (pResult->logOutput != nullptr)
        delete[] pResult->logOutput;

    return AnyDSL_SUCCESS;
}

AnyDSLResult JIT::lookup(AnyDSLJITModule module, const char* function, AnyDSLJITLookupInfo* pInfo)
{
    if (!jit().check_key(unwrapModule(module)))
        return (AnyDSL_INVALID_HANDLE);

    CHECK_RET_PTR(function);
    CHECK_RET_TYPE(pInfo, AnyDSL_STRUCTURE_TYPE_JIT_LOOKUP_INFO);

    void* func = jit().lookup_function(unwrapModule(module), function);

    if (func == nullptr)
        return HANDLE_ERROR(AnyDSL_JIT_NO_FUNCTION);

    pInfo->pHandle = func;

    return AnyDSL_SUCCESS;
}

AnyDSLResult JIT::link(AnyDSLJITModule module, size_t count, const AnyDSLJITLinkInfo* pLinkInfo)
{
    // module == AnyDSL_NULL_HANDLE -> for all
    unused(module);
    // if (!jit().check_key(unwrapModule(module)))
    //     return AnyDSL_INVALID_HANDLE;

    if (count == 0)
        return HANDLE_ERROR(AnyDSL_INVALID_VALUE);

    for (size_t i = 0; i < count; ++i) {
        CHECK_RET_TYPE(pLinkInfo, AnyDSL_STRUCTURE_TYPE_JIT_LINK_INFO);

        jit().link(pLinkInfo->libraryFilename);

        ++pLinkInfo;
    }

    return AnyDSL_SUCCESS;
}

} // namespace AnyDSLInternal
