#include <stdio.h>
#include <stdlib.h>
#include <llvm-c/Core.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Types.h>
#include "global_optimizer.h"
#include "local_optimizer.h"
#include "llvm_parser.h"

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
    global_optimizer(module);

    LLVMPrintModuleToFile(module, output, NULL);

    return 0;
}