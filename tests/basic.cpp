#include "test_base.hpp"

TEST(Basic) {
    luft::FunctionSignatureBuilder signature_builder;
    signature_builder.DefineOutput(luft::OutputPosition{
        .type = luft::msl_float4,
    });

    auto [function, meta] = signature_builder.CreateFunction(
        "main_vertex", context, module, 0, false);

    auto entry_bb = llvm::BasicBlock::Create(context, "entry", function);
    llvm::IRBuilder<> builder(entry_bb);
    builder.getFastMathFlags().setFast(true);

    // Body
    llvm::Value* ret_v = llvm::UndefValue::get(function->getReturnType());

    llvm::Value* position_v = llvm::UndefValue::get(types._float4);
    position_v = builder.CreateInsertElement(
        position_v, llvm::ConstantFP::get(builder.getFloatTy(), 0.0),
        builder.getInt32(0));
    position_v = builder.CreateInsertElement(
        position_v, llvm::ConstantFP::get(builder.getFloatTy(), 0.0),
        builder.getInt32(1));
    position_v = builder.CreateInsertElement(
        position_v, llvm::ConstantFP::get(builder.getFloatTy(), 0.5),
        builder.getInt32(2));
    position_v = builder.CreateInsertElement(
        position_v, llvm::ConstantFP::get(builder.getFloatTy(), 1.0),
        builder.getInt32(3));
    ret_v = builder.CreateInsertValue(ret_v, position_v, {0});

    builder.CreateRet(ret_v);

    module.getOrInsertNamedMetadata("air.vertex")->addOperand(meta);

    return 0;
}
