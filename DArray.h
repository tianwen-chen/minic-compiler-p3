#ifndef DARRAY_H
#define DARRAY_H

#include <llvm-c/Core.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Types.h>

#include <stdlib.h>  // for malloc, realloc, free

typedef struct {
    int capacity;
    int size;
    LLVMValueRef* data;
} D_Array;

D_Array* create_D_Array();
void push_back(D_Array* arr, LLVMValueRef value);
void delete_element(D_Array* arr, int index);
void copy_D_Array(D_Array* dest, D_Array* src);
void free_D_Array(D_Array* arr);
D_Array* union_D_Arrays(D_Array* a, D_Array* b);
D_Array* minus(D_Array* a, D_Array* b);

#endif /* DARRAY_H */
