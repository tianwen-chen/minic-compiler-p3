#include <stdio.h>
#include"Core.h"
#include <stdbool.h>
#include <unordered_map>
#include <cstdlib>
#include <vector>

using std::vector;
using std::unordered_map;

/* customized array */
typedef struct {
    int capacity;
    int size;
    LLVMValueRef* data;
} D_Array;

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

void free_D_Array(D_Array* arr) {
    free(arr->data);
    free(arr);
}

/* global vars */
unordered_map<LLVMBasicBlockRef, D_Array*> gen_dict;
unordered_map<LLVMBasicBlockRef, D_Array*> kill_dict;
unordered_map<LLVMBasicBlockRef, D_Array*> in_dict;
unordered_map<LLVMBasicBlockRef, D_Array*> out_dict;
unordered_map<LLVMBasicBlockRef, vector<LLVMBasicBlockRef>*> pred_dict;     // predecessors
unordered_map<LLVMBasicBlockRef, int> bb_indices;           // index basic blocks for debug purpose

/* global flags*/
bool in_out_change = true;
bool remove_load_change = true;
bool common_subexpression_elimination_change = true;
bool dead_code_elimination_change = true;
bool constant_folding_change = true;


/* func declarations */
D_Array* find_store_inst(LLVMBasicBlockRef block, LLVMValueRef addr);
void construct_predecessor_dict(LLVMModuleRef m);
/* local optimizations */
void common_subexpression_elimination(LLVMBasicBlockRef bb);
bool common_subexpression_elimination_safety_check(LLVMValueRef value1, LLVMValueRef value2);
void dead_code_elimination(LLVMBasicBlockRef bb);
void constant_folding(LLVMBasicBlockRef bb);
/* global optimization routines */
void compute_GEN_and_KILL(LLVMBasicBlockRef block, D_Array* gen, D_Array* kill);
void compute_IN_and_OUT(LLVMModuleRef m);
void remove_extra_load(LLVMBasicBlockRef block);
/* helper functions */
void print_pred_and_succ_dict(unordered_map<LLVMBasicBlockRef, vector<LLVMBasicBlockRef>*> dict);
void construct_bb_indices(LLVMModuleRef m);

/* global optimzation function -- only need to call this */
void global_optimizer(LLVMModuleRef m){
    // construct predecessors dict
    construct_predecessor_dict(m);
    // fix point
    bool change = true;
    while(change){
        // local optimizations
        LLVMValueRef first_func = LLVMGetFirstFunction(m);
        for(LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(first_func); block != NULL; block = LLVMGetNextBasicBlock(block)){
            common_subexpression_elimination(block);
            dead_code_elimination(block);
            constant_folding(block);
        }
        printf("after local optimizations\n");
        // compute GEN and KILL
        for(LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(first_func); block != NULL; block = LLVMGetNextBasicBlock(block)){
            D_Array* gen = create_D_Array();
            D_Array* kill = create_D_Array();
            compute_GEN_and_KILL(block, gen, kill);
            gen_dict[block] = gen;
            kill_dict[block] = kill;
        }
        // compute IN and OUT
        compute_IN_and_OUT(m);
        // constant propagation
        for(LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(first_func); block != NULL; block = LLVMGetNextBasicBlock(block)){
            remove_extra_load(block);
        }
        // check for fix point: all global flags false
        if(!in_out_change && !remove_load_change && !common_subexpression_elimination_change && !dead_code_elimination_change && !constant_folding_change){
            change = false;
        }
    }
}

/* global dict construction routines */

void compute_GEN_and_KILL(LLVMBasicBlockRef block, D_Array* gen, D_Array* kill) {
    D_Array* all_store = create_D_Array();
    LLVMValueRef i;
    // compute GEN
    for(i = LLVMGetFirstInstruction(block); i != NULL; i = LLVMGetNextInstruction(i)){
        if(LLVMGetInstructionOpcode(i) == LLVMStore){
            // add to store set
            push_back(all_store, i);
            // check if any other inst in gen is killed by i
            bool add = true;
            for(int j = 0; j <gen->size; j++){
                LLVMValueRef curr_inst = gen->data[j];
                // check if curr_inst is store and check the memory location
                if (LLVMGetInstructionOpcode(curr_inst) == LLVMStore && LLVMGetOperand(curr_inst, 1) == LLVMGetOperand(i, 1)){
                    add = false;
                    break;
                }
            }
            if (add){
                push_back(gen, i);
            }
        }
    }

    // copmpute KILL
    for(int i = 0; i < all_store->size; i++){
        LLVMValueRef store_inst = all_store->data[i];
        for(int j = 0; j < kill->size; j++){
            LLVMValueRef curr_inst = kill->data[j];
            // skip itself
            if(curr_inst == store_inst){
                continue;
            }
            if (LLVMGetOperand(curr_inst, 1) == LLVMGetOperand(store_inst, 1)){
                push_back(kill, curr_inst);
            }
        }
    }
    
}

void compute_IN_and_OUT(LLVMModuleRef m){
    bool change = true;
    int loop_count = 0;
    LLVMValueRef first_func = LLVMGetFirstFunction(m);
    /* initialize IN and OUT */
    for(LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(first_func); block != NULL; block = LLVMGetNextBasicBlock(block)){
        // in_dict[block] = create_D_Array();
        // for OUT, copy from gen[block]
        out_dict[block] = create_D_Array();
        for(int i = 0; i < gen_dict[block]->size; i++){
            push_back(out_dict[block], gen_dict[block]->data[i]);
        }
    }
    while(change){
        loop_count++;
        for(LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(first_func); block != NULL; block = LLVMGetNextBasicBlock(block)){
            D_Array* new_in = create_D_Array();
            // compute IN
            // IN[block] = union(OUT[P1], OUT[P2],.., OUT[PN]), where P1, P2, .. PN are predecessors of block
            for(int i = 0; i < pred_dict[block]->size(); i++){
                LLVMBasicBlockRef curr_pred = pred_dict[block]->at(i);
                new_in = union_D_Arrays(new_in, out_dict[curr_pred]);
            }
            // compute OUT
            D_Array* new_out = create_D_Array();
            new_out = union_D_Arrays(out_dict[block], (minus(new_in, kill_dict[block])));

            // check if out_dict[block] == new_out
            if (out_dict[block]->size != new_out->size){
                change = true;
            } else {
                // compare the two array having same elements using minus
                D_Array* diff = minus(out_dict[block], new_out);
                if (diff->size != 0){
                    change = true;
                } else{
                    change = false;
                    break;
                }
            }

            // update in and out
            out_dict[block] = new_out;
            in_dict[block] = new_in;
        }
    }
    // check for flag
    if (loop_count == 1){
        in_out_change = false;
        printf("no change for in and out\n");
    } else{
        printf("change for in and out\n");
    }
}

void construct_predecessor_dict(LLVMModuleRef m){
    // construct bb_indices
    construct_bb_indices(m);
    // first construct the successors dict
    unordered_map<LLVMBasicBlockRef, vector<LLVMBasicBlockRef>*> succ_dict;
    for(LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(LLVMGetFirstFunction(m)); block != NULL; block = LLVMGetNextBasicBlock(block)){
        // allocate memory for vector
        succ_dict[block] = new vector<LLVMBasicBlockRef>();
        // use LLVMGetNumSuccessors
        LLVMValueRef term = LLVMGetBasicBlockTerminator(block);
        for(int i = 0; i < LLVMGetNumSuccessors(term); i++){
            LLVMBasicBlockRef succ = LLVMGetSuccessor(term, i);
            succ_dict[block]->push_back(succ);
        }
    }
    // take care of memory allocation
    for(LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(LLVMGetFirstFunction(m)); block != NULL; block = LLVMGetNextBasicBlock(block)){
        pred_dict[block] = new vector<LLVMBasicBlockRef>();
    }
    // construct the predecessors dict by reversing the succ_dict
    for(LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(LLVMGetFirstFunction(m)); block != NULL; block = LLVMGetNextBasicBlock(block)){
        for(int i = 0; i < succ_dict[block]->size(); i++){
            LLVMBasicBlockRef succ = succ_dict[block]->at(i);            
            pred_dict[succ]->push_back(block);
        }
    }
    // free succ_dict
    for(LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(LLVMGetFirstFunction(m)); block != NULL; block = LLVMGetNextBasicBlock(block)){
        succ_dict[block]->clear();
    }
    // print_pred_and_succ_dict(succ_dict);
}

void construct_bb_indices(LLVMModuleRef m){
    int index = 0;
    for(LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(LLVMGetFirstFunction(m)); block != NULL; block = LLVMGetNextBasicBlock(block)){
        bb_indices[block] = index;
        index++;
    }
}


/* helper func */

void print_pred_and_succ_dict(unordered_map<LLVMBasicBlockRef, vector<LLVMBasicBlockRef>*> dict){
    for(auto it = dict.begin(); it != dict.end(); it++){
        printf("Block: %d\n", bb_indices[it->first]);
        printf("Predecessors: ");
        for(int i = 0; i < it->second->size(); i++){
            printf("%d ", bb_indices[it->second->at(i)]);
        }
        printf("\n");
    }
}

/* remove_extra_load function */
void remove_extra_load(LLVMBasicBlockRef block){
    D_Array* marked_inst = create_D_Array();
    D_Array* i_list = create_D_Array();
    copy_D_Array(i_list, gen_dict[block]);
    for(LLVMValueRef inst = LLVMGetFirstInstruction(block); inst != NULL; inst = LLVMGetNextInstruction(inst)){
        // if is store, add to i_list, remove all stores in i_list that are killed by inst
        // REVIEW -  double add load inst?
        if(LLVMGetInstructionOpcode(inst) == LLVMStore){
            push_back(i_list, inst);
            for(int i = 0; i < i_list->size; i++){
                LLVMValueRef curr_inst = i_list->data[i];
                if(LLVMGetInstructionOpcode(curr_inst) == LLVMStore && LLVMGetOperand(curr_inst, 1) == LLVMGetOperand(inst, 1)){
                    delete_element(i_list, i);
                }
            }
        }
        if(LLVMGetInstructionOpcode(inst) == LLVMLoad){
            // get the address of the load inst
            LLVMValueRef addr = LLVMGetOperand(inst, 0);
            // find all strores in i_list that writes to addr
            D_Array* store_inst = find_store_inst(block, addr);
            // if there is no store inst that writes to addr, skip
            if(store_inst->size == 0){
                continue;
            }
            // check if all these store instructions are constant store and store the same constant
            bool same = true;
            LLVMValueRef constant = LLVMGetOperand(store_inst->data[0], 0);
            if(LLVMIsAConstant(constant)){
                for(int i = 0; i < store_inst->size; i++){
                    LLVMValueRef store = store_inst->data[i];
                    if(LLVMGetOperand(store, 0) != constant){
                        same = false;
                        break;
                    }
                }
            }
            // FIXME - use LLVMConstInt & LLVMConstIntGetSExtValue? replace all uses if inst by the constant
            if(same){
                LLVMReplaceAllUsesWith(inst, constant);
                push_back(marked_inst, inst);
            }
        }
    }
    // delete all the marked load instruction
    for(int i = 0; i < marked_inst->size; i++){
        LLVMValueRef inst = marked_inst->data[i];
        LLVMInstructionEraseFromParent(inst);
    }

    // check for flag
    if(marked_inst->size == 0){
        remove_load_change = false;
    }
}

/* helper func: find all store inst that write to a specific address*/
// REVIEW - check the type of the address
D_Array* find_store_inst(LLVMBasicBlockRef block, LLVMValueRef addr){
    D_Array* res = create_D_Array();
    for(LLVMValueRef inst = LLVMGetFirstInstruction(block); inst != NULL; inst = LLVMGetNextInstruction(inst)){
        if(LLVMGetInstructionOpcode(inst) == LLVMStore && LLVMGetOperand(inst, 1) == addr){
            push_back(res, inst);
        }
    }
    return res;
}




/* local optimizations */
void common_subexpression_elimination(LLVMBasicBlockRef bb) {
    // loop over each pair of instructions in the basic block
    LLVMValueRef instruction_1;
    bool flag = false;
    for (instruction_1 = LLVMGetFirstInstruction(bb); instruction_1 != NULL; instruction_1 = LLVMGetNextInstruction(instruction_1)) {
        LLVMValueRef instruction_2;
        for(instruction_2 = LLVMGetNextInstruction(instruction_1); instruction_2 != NULL; instruction_2 = LLVMGetNextInstruction(instruction_2)) {
            // check if the two instructions have the same opcode and the same operands
            if(LLVMGetInstructionOpcode(instruction_1) == LLVMGetInstructionOpcode(instruction_2)) {
                // check if the two instructions have the same number of operands
                if(LLVMGetNumOperands(instruction_1) == LLVMGetNumOperands(instruction_2)) {
                    // check if the two instructions have the same operands
                    bool same_operands = true;
                    for(int i = 0; i < LLVMGetNumOperands(instruction_1); i++) {
                        if(LLVMGetOperand(instruction_1, i) != LLVMGetOperand(instruction_2, i)) {
                            same_operands = false;
                            break;
                        }
                    }
                    // if opcode is load, do the safety check
                    if(LLVMGetInstructionOpcode(instruction_1) == LLVMLoad) {
                        if(!common_subexpression_elimination_safety_check(instruction_1, instruction_2)) {
                            same_operands = false;
                        }
                    }
                    if(same_operands) {
                        // replace all uses of instruction_2 with instruction_1
                        LLVMReplaceAllUsesWith(instruction_2, instruction_1);
                        // mark flag
                        flag = true;
                    }
                }
            }
        }
    }
    // check for global flag
    if(!flag){
        common_subexpression_elimination_change = false;
    }
}

bool common_subexpression_elimination_safety_check(LLVMValueRef value1, LLVMValueRef value2) {
    // get the address of the load operation
    LLVMValueRef address = LLVMGetOperand(value1, 0);

    LLVMValueRef next_instruction;
    for(next_instruction = LLVMGetNextInstruction(value1); next_instruction != value2; next_instruction = LLVMGetNextInstruction(next_instruction)) {
        if(LLVMGetInstructionOpcode(next_instruction) == LLVMStore) {
            if(LLVMGetOperand(next_instruction, 1) == address) {
                return false;
            }
        }
    }

    return true;
}

void dead_code_elimination(LLVMBasicBlockRef bb) {
    bool change = false;
    // TODO: complete the list in header file (or macro?)
    LLVMOpcode instructions_to_keep[3] = {LLVMStore, LLVMCall, LLVMRet};
    // loop over each instruction in the basic block and check if the oprand is used by any other instruction
    LLVMValueRef instruction;
    for (instruction = LLVMGetFirstInstruction(bb); instruction != NULL; instruction = LLVMGetNextInstruction(instruction)) {
        // check if the instruction is in the list of instructions to keep
        bool keep = false;
        // TODO: number of loops (size of the list), or use a hashset?
        for(int i = 0; i < 3; i++) {
            if(LLVMGetInstructionOpcode(instruction) == instructions_to_keep[i]) {
                keep = true;
                break;
            }
        }
        if(!keep) {
            // check if the instruction is used by any other instruction
            if(LLVMGetFirstUse(instruction) == NULL) {
                // if not, remove the instruction
                LLVMInstructionEraseFromParent(instruction);
                change = true;
            }
        }
    }
    // check for global flag
    if(!change){
        dead_code_elimination_change = false;
    }
}

void constant_folding(LLVMBasicBlockRef bb) {
    bool change = false;
    LLVMValueRef instruction;
    for (instruction = LLVMGetFirstInstruction(bb); instruction != NULL; instruction = LLVMGetNextInstruction(instruction)) {
        // check if the instruction is an arithmetic operation
        if(LLVMGetInstructionOpcode(instruction) == LLVMAdd || LLVMGetInstructionOpcode(instruction) == LLVMSub || LLVMGetInstructionOpcode(instruction) == LLVMMul) {
            // check if all operands are constants
            bool all_constants = true;
            for(int i = 0; i < LLVMGetNumOperands(instruction); i++) {
                if(!LLVMIsConstant(LLVMGetOperand(instruction, i))) {
                    all_constants = false;
                    break;
                }
            }
            if(all_constants) {
                // calculate the result of the arithmetic operation
                LLVMValueRef result;
                if(LLVMGetInstructionOpcode(instruction) == LLVMAdd) {
                    result = LLVMConstAdd(LLVMGetOperand(instruction, 0), LLVMGetOperand(instruction, 1));
                } else if(LLVMGetInstructionOpcode(instruction) == LLVMSub) {
                    result = LLVMConstSub(LLVMGetOperand(instruction, 0), LLVMGetOperand(instruction, 1));
                } else if(LLVMGetInstructionOpcode(instruction) == LLVMMul) {
                    result = LLVMConstMul(LLVMGetOperand(instruction, 0), LLVMGetOperand(instruction, 1));
                }
                // replace all uses of the instruction with the result
                LLVMReplaceAllUsesWith(instruction, result);
                change = true;
            }
        }
    }

    // check for global flag
    if(!change){
        constant_folding_change = false;
    }
}