#include <stdio.h>
#include <stdlib.h>
#include <llvm-c/Core.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Types.h>
#include "global_optimizer.h"
#include "local_optimizer.h"
#include "llvm_parser.h"

int main(int argc, char **argv){
    // get the parameter (ll file) and create a llvm module out of it
    if(argc != 2) {
        printf("You forgot something \n");
        return 1;
    }
    char *ll_file = argv[1];
    LLVMModuleRef module = createLLVMModel(ll_file);
	
    // global optimizations
    global_optimizer(module);

    // print the optimized module
	char *out_file = (char *)malloc(strlen(ll_file) - 2);
	strncpy(out_file, ll_file, strlen(ll_file) - 3);
	strcat(out_file, "out.ll");

    LLVMPrintModuleToFile(module, "global_optimized.ll", NULL);

	free(out_file);
    return 0;
}