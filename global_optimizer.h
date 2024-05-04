#ifndef GLOBAL_OPTIMIZER_H
#define GLOBAL_OPTIMIZER_H

#include <stdio.h>
#include <stdbool.h>
#include <unordered_map>
#include <cstdlib>
#include <vector>

#include <llvm-c/Core.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Types.h>

#include "local_optimizer.h"
#include "DArray.h"

using std::vector;
using std::unordered_map;

/* Declarations */
void global_optimizer(LLVMModuleRef m);

#endif /* GLOBAL_OPTIMIZER_H */
