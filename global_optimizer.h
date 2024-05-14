#ifndef GLOBAL_OPTIMIZER_H
#define GLOBAL_OPTIMIZER_H

#include <llvm-c/Core.h>
#include <llvm-c/Types.h>

void optimizer(LLVMModuleRef m);

/* local optimizations */
bool common_subexpression_elimination(LLVMBasicBlockRef bb);
bool dead_code_elimination(LLVMBasicBlockRef bb);
bool constant_folding(LLVMBasicBlockRef bb);

#endif // GLOBAL_OPTIMIZER_H