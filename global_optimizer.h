#ifndef GLOBAL_OPTIMIZER_H
#define GLOBAL_OPTIMIZER_H

#include <llvm-c/Core.h>
#include <llvm-c/Types.h>

void global_optimizer(LLVMModuleRef m);

#endif // GLOBAL_OPTIMIZER_H