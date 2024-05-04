#ifndef LOCAL_OPTIMIZER_H
#define LOCAL_OPTIMIZER_H

#include <llvm-c/Core.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Types.h>

#include <stdbool.h>  // for bool type

/* local optimizations */
void common_subexpression_elimination(LLVMBasicBlockRef bb);
void dead_code_elimination(LLVMBasicBlockRef bb);
void constant_folding(LLVMBasicBlockRef bb);

// Global flags for changes made during optimization
extern bool common_subexpression_elimination_change;
extern bool dead_code_elimination_change;
extern bool constant_folding_change;

#endif /* LOCAL_OPTIMIZER_H */