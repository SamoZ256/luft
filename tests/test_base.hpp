#pragma once

#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Passes/OptimizationLevel.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/VersionTuple.h"
#include "llvm/Transforms/Scalar/Scalarizer.h"

#include "luft/luft.hpp"

struct ModuleOptions {
    bool fast_math_enabled;
};

class TestBase {
  public:
    TestBase(const std::string_view& name_, const ModuleOptions& opts)
        : name{name_}, module("default", context), types(context) {
        // TODO: diagnostics handler

        // TODO: move this to the API
        module.setSourceFileName("airconv_generated.metal");
        module.setTargetTriple("air64-apple-macosx14.0.0");
        module.setDataLayout(
            "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:"
            "64-f32:32:32-f64:"
            "64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:"
            "64-v96:128:128-"
            "v128:128:128-v192:256:256-v256:256:256-v512:512:512-"
            "v1024:1024:1024-n8:"
            "16:32");
        module.setSDKVersion(llvm::VersionTuple(14, 0));
        module.addModuleFlag(llvm::Module::ModFlagBehavior::Error, "wchar_size",
                             4);
        module.addModuleFlag(llvm::Module::ModFlagBehavior::Max,
                             "frame-pointer", 2);
        module.addModuleFlag(llvm::Module::ModFlagBehavior::Max,
                             "air.max_device_buffers", 31);
        module.addModuleFlag(llvm::Module::ModFlagBehavior::Max,
                             "air.max_constant_buffers", 31);
        module.addModuleFlag(llvm::Module::ModFlagBehavior::Max,
                             "air.max_threadgroup_buffers", 31);
        module.addModuleFlag(llvm::Module::ModFlagBehavior::Max,
                             "air.max_textures", 128);
        module.addModuleFlag(llvm::Module::ModFlagBehavior::Max,
                             "air.max_read_write_textures", 8);
        module.addModuleFlag(llvm::Module::ModFlagBehavior::Max,
                             "air.max_samplers", 16);

        auto createUnsignedInteger = [&](uint32_t s) {
            return llvm::ConstantAsMetadata::get(
                llvm::ConstantInt::get(context, llvm::APInt{32, s, false}));
        };
        auto createString = [&](auto s) {
            return llvm::MDString::get(context, s);
        };

        auto airVersion = module.getOrInsertNamedMetadata("air.version");
        airVersion->addOperand(llvm::MDTuple::get(
            context, {createUnsignedInteger(2), createUnsignedInteger(6),
                      createUnsignedInteger(0)}));
        auto airLangVersion =
            module.getOrInsertNamedMetadata("air.language_version");
        airLangVersion->addOperand(llvm::MDTuple::get(
            context, {createString("Metal"), createUnsignedInteger(3),
                      createUnsignedInteger(0), createUnsignedInteger(0)}));

        auto airCompileOptions =
            module.getOrInsertNamedMetadata("air.compile_options");
        airCompileOptions->addOperand(llvm::MDTuple::get(
            context, {createString("air.compile.denorms_disable")}));
        airCompileOptions->addOperand(llvm::MDTuple::get(
            context, {opts.fast_math_enabled
                          ? createString("air.compile.fast_math_enable")
                          : createString("air.compile.fast_math_disable")}));
        airCompileOptions->addOperand(llvm::MDTuple::get(
            context, {createString("air.compile.framebuffer_fetch_enable")}));
    };

    void RunOptimizationPasses(llvm::OptimizationLevel opt) {
        /*
                if (!llvm_overwrite.test_and_set()) {
                    auto Map = cl::getRegisteredOptions();
                    auto InfiniteLoopThreshold =
                        Map["instcombine-infinite-loop-threshold"];
                    if (InfiniteLoopThreshold) {
                        reinterpret_cast<cl::opt<unsigned>*>(InfiniteLoopThreshold)
                            ->setValue(1000);
                    }
                }
                */

        // Create the analysis managers.
        // These must be declared in this order so that they are destroyed in
        // the correct order due to inter-analysis-manager references.
        llvm::LoopAnalysisManager LAM;
        llvm::FunctionAnalysisManager FAM;
        llvm::CGSCCAnalysisManager CGAM;
        llvm::ModuleAnalysisManager MAM;

        // Create the new pass manager builder.
        // Take a look at the PassBuilder constructor parameters for more
        // customization, e.g. specifying a TargetMachine or various debugging
        // options.
        llvm::PassBuilder PB;

        // Register all the basic analyses with the managers.
        PB.registerModuleAnalyses(MAM);
        PB.registerCGSCCAnalyses(CGAM);
        PB.registerFunctionAnalyses(FAM);
        PB.registerLoopAnalyses(LAM);
        PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

        llvm::ModulePassManager MPM = PB.buildPerModuleDefaultPipeline(opt);

        llvm::FunctionPassManager FPM;
        FPM.addPass(llvm::ScalarizerPass());

        MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
        MPM.addPass(llvm::VerifierPass());

        // Optimize the IR!
        MPM.run(module, MAM);
    }

    int Run() {
        int res = RunImpl();
        if (res != 0)
            return res;

        // TODO: enable optimizations
        // RunOptimizationPasses(llvm::OptimizationLevel::O2);

        module.print(llvm::outs(), nullptr);

        /*
        llvm::SmallVector<char, 0> vec;
        llvm::raw_svector_ostream os(vec);
        luft::metallib::MetallibWriter writer;
        writer.Write(module, os);
        */

        // TODO: verify the output

        return 0;
    }

  protected:
    std::string name;

    llvm::LLVMContext context;
    llvm::Module module;

    luft::AirType types;

    virtual int RunImpl() = 0;
};

#define TEST(name)                                                             \
    class Test##name : public TestBase {                                       \
      public:                                                                  \
        Test##name() : TestBase(#name, {.fast_math_enabled = true}) {}         \
        int RunImpl() override;                                                \
    };                                                                         \
    int main() {                                                               \
        Test##name test;                                                       \
        return test.Run();                                                     \
    }                                                                          \
    int Test##name::RunImpl()
