#include"DArray.h"

D_Array* create_D_Array() {
    D_Array* arr = (D_Array*) malloc(sizeof(D_Array));
    arr->capacity = 10;
    arr->size = 0;
    arr->data = (LLVMValueRef*) malloc(arr->capacity * sizeof(LLVMValueRef));
    return arr;
}

void push_back(D_Array* arr, LLVMValueRef value) {
    if (arr->size == arr->capacity) {
        arr->capacity *= 2;
        arr->data = (LLVMValueRef*) realloc(arr->data, arr->capacity * sizeof(LLVMValueRef));
    }
    arr->data[arr->size++] = value;
}

void delete_element(D_Array* arr, int index) {
    for (int i = index; i < arr->size - 1; i++) {
        arr->data[i] = arr->data[i + 1];
    }
    arr->size--;
}

void free_D_Array(D_Array* arr) {
    free(arr->data);
    free(arr);
}
