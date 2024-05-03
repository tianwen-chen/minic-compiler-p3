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


int main(int argc, char **argv){
    // get the parameter (ll file) and create a llvm module out of it
    if(argc != 2) {
        printf("Usage: %s <ll file>\n", argv[0]);
        return 1;
    }
    char *ll_file = argv[1];
    LLVMModuleRef module = createLLVMModel(ll_file);
    // global optimizations
    global_optimizer(module);
    // print the optimized module
    LLVMPrintModuleToFile(module, "global_optimized.ll", NULL);
    return 0;
}