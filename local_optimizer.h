#include <llvm-c/Core.h>
#include <stdbool.h>

#ifndef LOCAL_OPTIMIZER_H
#define LOCAL_OPTIMIZER_H

// FIXME define the instructions_to_keep list here: store, function calls, memory allocation & deallocation, I/O operations
// #define INSTRUCTIONS_TO_KEEP_SIZE 3
// LLVMOpcode instructions_to_keep[INSTRUCTIONS_TO_KEEP_SIZE] = {LLVMStore, LLVMCall, LLVMRet};


LLVMBasicBlockRef common_subexpression_elimination(LLVMBasicBlockRef bb);

LLVMBasicBlockRef dead_code_elimination(LLVMBasicBlockRef bb);

LLVMBasicBlockRef constant_folding(LLVMBasicBlockRef bb);

bool common_subexpression_elimination_safety_check(LLVMValueRef value1, LLVMValueRef value2);

#endif // LOCAL_OPTIMIZER_H