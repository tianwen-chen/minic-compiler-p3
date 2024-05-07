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

void copy_D_Array(D_Array* dest, D_Array* src) {
    dest->capacity = src->capacity;
    dest->size = src->size;
    dest->data = (LLVMValueRef*) malloc(dest->capacity * sizeof(LLVMValueRef));
    for (int i = 0; i < src->size; i++) {
        dest->data[i] = src->data[i];
    }
}

void free_D_Array(D_Array* arr) {
    free(arr->data);
    free(arr);
}


// todo: free memory? 
/* a union b */
D_Array* union_D_Arrays(D_Array* a, D_Array* b){
    D_Array* res = create_D_Array();
    for(int i = 0; i < a->size; i++){
        push_back(res, a->data[i]);
    }
    for(int i = 0; i < b->size; i++){
        bool add = true;
        for(int j = 0; j < a->size; j++){
            if (a->data[j] == b->data[i]){
                add = false;
                break;
            }
        }
        if (add){
            push_back(res, b->data[i]);
        }
    }
    return res;
}

/* a - b */
D_Array* minus(D_Array* a, D_Array* b){
    D_Array* res = create_D_Array();
    for(int i = 0; i < a->size; i++){
        bool add = true;
        for(int j = 0; j < b->size; j++){
            if (a->data[i] == b->data[j]){
                add = false;
                break;
            }
        }
        if (add){
            push_back(res, a->data[i]);
        }
    }
    return res;
}