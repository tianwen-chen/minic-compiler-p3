/*
 test for local_optimizer.c
*/
#include <stdio.h>
#include <stdlib.h>
#include "global_optimizer.h"
#include <llvm-c/Core.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Types.h>

LLVMModuleRef createLLVMModel(char * filename){
	char *err = 0;

	LLVMMemoryBufferRef ll_f = 0;
	LLVMModuleRef m = 0;

	LLVMCreateMemoryBufferWithContentsOfFile(filename, &ll_f, &err);

	if (err != NULL) { 
		printf("Error: %s\n", err);
		return NULL;
	}
	
	LLVMParseIRInContext(LLVMGetGlobalContext(), ll_f, &m, &err);

	if (err != NULL) {
		printf("Error: %s\n", err);
	}

	return m;
}


int main(int argc, char *argv[]){
    // get the parameter (ll file) and create a llvm module out of it
    if(argc != 3) {
        printf("You forgot something \n");
        return 1;
    }
    char *input = argv[1];
	char *output = argv[2];
	LLVMModuleRef module = createLLVMModel(input);
	
    // global optimizations
    optimizer(module);

	// local optimizations
	// bool change = true;
    // while(change){
    //     // local optimizations
    //     LLVMValueRef first_func = LLVMGetFirstFunction(module);
    //     bool common_subexpression_elimination_change = false;
    //     bool dead_code_elimination_change = false;
    //     bool constant_folding_change = false;

    //     for(LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(first_func); block != NULL; block = LLVMGetNextBasicBlock(block)){
    //         common_subexpression_elimination_change = common_subexpression_elimination(block);
    //         dead_code_elimination_change = dead_code_elimination(block);
    //         constant_folding_change = constant_folding(block);
    //     }
    //     // check for fix point: all global flags false
    //     if(!common_subexpression_elimination_change && !dead_code_elimination_change && !constant_folding_change){
    //         change = false;
    //     }
    // }

    LLVMPrintModuleToFile(module, output, NULL);

    return 0;
}