/* customized class for dynamic array */
#ifndef DARRAY_H
#define DARRAY_H

#include <stdlib.h>
#include"Core.h"

typedef struct {
    int capacity;
    int size;
    LLVMValueRef* data;
} D_Array;

D_Array* create_D_Array();
void push_back(D_Array* arr, LLVMValueRef value);
void delete_element(D_Array* arr, int index);
void free_D_Array(D_Array* arr);

#endif // DARRAY_H