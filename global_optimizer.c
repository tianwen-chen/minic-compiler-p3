#include <stdio.h>
#include"Core.h"
#include <stdbool.h>
#include <unordered_map>
#include <cstdlib>
#include <vector>
#include <algorithm>

using std::vector;
using std::unordered_map;


/* TODO LIST
    1. change all D_array to vector
    2. GDB/print into in_dict to see if empty is valid (check the result in last loop)
    3. valgrind
*/

/* global vars */
unordered_map<LLVMBasicBlockRef, vector<LLVMValueRef>*> gen_dict;
unordered_map<LLVMBasicBlockRef, vector<LLVMValueRef>*> kill_dict;
unordered_map<LLVMBasicBlockRef, vector<LLVMValueRef>*> in_dict;
unordered_map<LLVMBasicBlockRef, vector<LLVMValueRef>*> out_dict;
unordered_map<LLVMBasicBlockRef, vector<LLVMBasicBlockRef>*> pred_dict;     // predecessors
unordered_map<LLVMBasicBlockRef, int> bb_indices;           // index basic blocks for debug purpose

/* global optimization final function */
void optimizer(LLVMModuleRef m);
/* local optimizations */
bool common_subexpression_elimination(LLVMBasicBlockRef bb);
bool common_subexpression_elimination_safety_check(LLVMValueRef value1, LLVMValueRef value2);
bool dead_code_elimination(LLVMBasicBlockRef bb);
bool constant_folding(LLVMBasicBlockRef bb);
/* global optimization routines */
void compute_GEN_and_KILL(LLVMBasicBlockRef block, vector<LLVMValueRef>* all_store);
void compute_IN_and_OUT(LLVMModuleRef m);
bool remove_extra_load(LLVMBasicBlockRef block);
bool global_optimization(LLVMModuleRef m);
/* helper functions */
void print_pred_and_succ_dict(unordered_map<LLVMBasicBlockRef, vector<LLVMBasicBlockRef>*> dict);
void construct_bb_indices(LLVMModuleRef m);
vector<LLVMValueRef>* find_store_inst(vector<LLVMValueRef>*, LLVMValueRef addr);
void construct_predecessor_dict(LLVMModuleRef m);
vector<LLVMValueRef>* union_vector(vector<LLVMValueRef>* a, vector<LLVMValueRef>* b);
vector<LLVMValueRef>* minus(vector<LLVMValueRef>* a, vector<LLVMValueRef>* b);
void copy_vector(vector<LLVMValueRef>* dest, vector<LLVMValueRef>* src);
bool are_equal_ignore_order(std::vector<LLVMValueRef>* vec1, std::vector<LLVMValueRef>* vec2);

/* global optimzation function -- only need to call this */
void optimizer(LLVMModuleRef m){
    // initialize global vars
    gen_dict = unordered_map<LLVMBasicBlockRef, vector<LLVMValueRef>*>();
    kill_dict = unordered_map<LLVMBasicBlockRef, vector<LLVMValueRef>*>();
    in_dict = unordered_map<LLVMBasicBlockRef, vector<LLVMValueRef>*>();
    out_dict = unordered_map<LLVMBasicBlockRef, vector<LLVMValueRef>*>();
    pred_dict = unordered_map<LLVMBasicBlockRef, vector<LLVMBasicBlockRef>*>();
    bb_indices = unordered_map<LLVMBasicBlockRef, int>();
    
    for(LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(LLVMGetFirstFunction(m)); block != NULL; block = LLVMGetNextBasicBlock(block)){
        gen_dict[block] = new vector<LLVMValueRef>();
        kill_dict[block] = new vector<LLVMValueRef>();
        in_dict[block] = new vector<LLVMValueRef>();
        out_dict[block] = new vector<LLVMValueRef>();
        pred_dict[block] = new vector<LLVMBasicBlockRef>();
    }
    // construct predecessors dict
    construct_predecessor_dict(m);
    // fix point
    bool change = true;
    while(change){
        // local optimizations
        LLVMValueRef first_func = LLVMGetFirstFunction(m);
        bool common_subexpression_elimination_change = false;
        bool dead_code_elimination_change = false;
        bool constant_folding_change = false;

        for(LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(first_func); block != NULL; block = LLVMGetNextBasicBlock(block)){
            common_subexpression_elimination_change = common_subexpression_elimination(block);
            dead_code_elimination_change = dead_code_elimination(block);
            constant_folding_change = constant_folding(block);
        }
        bool remove_load_change = global_optimization(m);
        // check for fix point: all global flags false
        if(!remove_load_change && !common_subexpression_elimination_change && !dead_code_elimination_change && !constant_folding_change){
            change = false;
        }
    }
    printf("Optimization done\n");
    // free all global vars
    for(auto it = gen_dict.begin(); it != gen_dict.end(); it++){
        delete it->second;
    }
    for(auto it = kill_dict.begin(); it != kill_dict.end(); it++){
        delete it->second;
    }
    for(auto it = in_dict.begin(); it != in_dict.end(); it++){
        delete it->second;
    }
    for(auto it = out_dict.begin(); it != out_dict.end(); it++){
        delete it->second;
    }
    for(auto it = pred_dict.begin(); it != pred_dict.end(); it++){
        delete it->second;
    }
    // free bb_indices
    bb_indices.clear();
    return;
}

bool global_optimization(LLVMModuleRef m){
    bool res = false;
    LLVMValueRef first_func = LLVMGetFirstFunction(m);
    // compute all_store
    vector<LLVMValueRef>* all_store = new vector<LLVMValueRef>();
    for(LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(first_func); block != NULL; block = LLVMGetNextBasicBlock(block)){
        for(LLVMValueRef inst = LLVMGetFirstInstruction(block); inst != NULL; inst = LLVMGetNextInstruction(inst)){
            if(LLVMIsAStoreInst(inst)){
                all_store->push_back(inst);
            }
        }
    }
    // compute GEN and KILL
    for(LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(first_func); block != NULL; block = LLVMGetNextBasicBlock(block)){   
        compute_GEN_and_KILL(block, all_store);
        /*
        printf("gen: ");
        printf("\n");
        for(int i = 0; i < gen_dict[block]->size(); i++){
            LLVMDumpValue(gen_dict[block]->at(i));
            printf("\n");
        }
        printf("\n");
        printf("kill: ");
        printf("\n");
        for(int i = 0; i < kill_dict[block]->size(); i++){
            LLVMDumpValue(kill_dict[block]->at(i));
            printf("\n");
        }
        printf("\n");
        */
    }
    compute_IN_and_OUT(m);
    // constant propagation
    for(LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(first_func); block != NULL; block = LLVMGetNextBasicBlock(block)){
        if(remove_extra_load(block)){
            res = true;
        }
    }

    return res;
}

/* global dict construction routines */

void compute_GEN_and_KILL(LLVMBasicBlockRef block, vector<LLVMValueRef>* all_store) {    
    // GEN: initialize gen to be empty
    vector<LLVMValueRef>* new_gen = new vector<LLVMValueRef>();
    // KILL: compute the set of all store instructions in the block
    LLVMValueRef i;
    // compute GEN & construct all_store
    for(i = LLVMGetFirstInstruction(block); i != NULL; i = LLVMGetNextInstruction(i)){
        if(LLVMIsAStoreInst(i)){            
            // GEN: check if any other inst in new_gen is killed by i
            bool add = true;
            LLVMValueRef curr_inst;
            for(int j = 0; j < new_gen->size(); j++){
                curr_inst = new_gen->at(j);
                // check if curr_inst is store to the same memory location as i
                if (LLVMGetOperand(curr_inst, 1) == LLVMGetOperand(i, 1)){
                    add = false;
                    break;
                }
                
            }
            // GEN: add i to new_gen if not killed by any other inst in new_gen
            if (!add){
                // delete curr_inst from new_gen
                new_gen->erase(std::remove(new_gen->begin(), new_gen->end(), curr_inst), new_gen->end());
            }
            new_gen->push_back(i);
        }
    }
    gen_dict[block] = new_gen;

    // copmpute KILL
    // initialize kill to be empty
    vector<LLVMValueRef>* new_kill = new vector<LLVMValueRef>();
    for(i = LLVMGetFirstInstruction(block); i != NULL; i = LLVMGetNextInstruction(i)){
        if(LLVMIsAStoreInst(i)){
            // add all the instructions in all_store that get killed by i to kill
            for(int j = 0; j < all_store->size(); j++){
                LLVMValueRef curr_inst = all_store->at(j);
                // skip itself
                if(curr_inst == i){
                    continue;
                }
                // REVIEW - check if the way of getting memory location is right
                if (LLVMGetOperand(curr_inst, 1) == LLVMGetOperand(i, 1)){
                    new_kill->push_back(curr_inst);
                }
            }
        }
    }
    kill_dict[block] = new_kill;
    // print gen and kill
    
    
}

void compute_IN_and_OUT(LLVMModuleRef m){

    bool change = true;
    int loop_count = 0;
    LLVMValueRef first_func = LLVMGetFirstFunction(m);
    /* initialize IN and OUT */

    // initialize IN for each block to be empty
    for(LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(first_func); block != NULL; block = LLVMGetNextBasicBlock(block)){
        in_dict[block] = new vector<LLVMValueRef>();
    }
    // initialize OUT for each block to be GEN
    for(LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(first_func); block != NULL; block = LLVMGetNextBasicBlock(block)){
        vector<LLVMValueRef>* out = new vector<LLVMValueRef>();
        copy_vector(out, gen_dict[block]);
    }

    while(change){
        loop_count++;
        for(LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(first_func); block != NULL; block = LLVMGetNextBasicBlock(block)){
            vector<LLVMValueRef>* in = new vector<LLVMValueRef>();
            // compute IN
            // IN[block] = union(OUT[P1], OUT[P2],.., OUT[PN]), where P1, P2, .. PN are predecessors of block
            for(int i = 0; i < pred_dict[block]->size(); i++){
                LLVMBasicBlockRef curr_pred = pred_dict[block]->at(i);
                in = union_vector(in, out_dict[curr_pred]);
            }
            // compute OUT
            vector<LLVMValueRef>* new_out = new vector<LLVMValueRef>();
            new_out = union_vector(gen_dict[block], (minus(in, kill_dict[block])));

            // check if out_dict[block] == new_out
            if (out_dict[block]->size() != new_out->size()){
                change = true;
            } else {
                // compare if new_out and out_dict[block] have the same elements
                if (!are_equal_ignore_order(new_out, out_dict[block])){
                    change = true;
                } else {
                    change = false;
                } 
            }

            // update in and out
            in_dict[block] = in;
            out_dict[block] = new_out;
        }
    }
    // go over every block and print the IN and OUT
    // llvmdumpvalue
    /*
    printf("----------------\n");
    for(LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(first_func); block != NULL; block = LLVMGetNextBasicBlock(block)){
        printf("Block:\n");
        printf("IN: ");
        printf("\n");
        for(int i = 0; i < in_dict[block]->size(); i++){
            LLVMDumpValue(in_dict[block]->at(i));
            printf("\n");
        }
        printf("\n");
        printf("OUT: ");
        printf("\n");
        for(int i = 0; i < out_dict[block]->size(); i++){
            LLVMDumpValue(out_dict[block]->at(i));
            printf("\n");
        }
        printf("\n");
    }
    printf("----------------\n");
    */
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
vector<LLVMValueRef>* union_vector(vector<LLVMValueRef>* a, vector<LLVMValueRef>* b){
    vector<LLVMValueRef>* res = new vector<LLVMValueRef>();
    for(int i = 0; i < a->size(); i++){
        res->push_back(a->at(i));
    }
    for(int i = 0; i < b->size(); i++){
        bool add = true;
        for(int j = 0; j < a->size(); j++){
            if(a->at(j) == b->at(i)){
                add = false;
                break;
            }
        }
        if(add){
            res->push_back(b->at(i));
        }
    }
    return res;
}

vector<LLVMValueRef>* minus(vector<LLVMValueRef>* a, vector<LLVMValueRef>* b){
    vector<LLVMValueRef>* res = new vector<LLVMValueRef>();
    for(int i = 0; i < a->size(); i++){
        bool add = true;
        for(int j = 0; j < b->size(); j++){
            if(a->at(i) == b->at(j)){
                add = false;
                break;
            }
        }
        if(add){
            res->push_back(a->at(i));
        }
    }
    return res;
}

void copy_vector(vector<LLVMValueRef>* dest, vector<LLVMValueRef>* src){
    for(int i = 0; i < src->size(); i++){
        dest->push_back(src->at(i));
    }
}

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

bool are_equal_ignore_order(std::vector<LLVMValueRef>* vec1, std::vector<LLVMValueRef>* vec2) {
    if (vec1->size() != vec2->size()) {
        return false;
    }

    // Make copies of the vectors because sorting is in-place
    std::vector<LLVMValueRef> copy1 = *vec1;
    std::vector<LLVMValueRef> copy2 = *vec2;

    // Sort both vectors
    std::sort(copy1.begin(), copy1.end());
    std::sort(copy2.begin(), copy2.end());

    // Compare the vectors
    return std::equal(copy1.begin(), copy1.end(), copy2.begin());
}

/* remove_extra_load function */
bool remove_extra_load(LLVMBasicBlockRef block){
    // R = IN[B]
    vector<LLVMValueRef>* r_set = new vector<LLVMValueRef>();
    copy_vector(r_set, in_dict[block]);

    vector<LLVMValueRef>* marked_load = new vector<LLVMValueRef>();
    
    for(LLVMValueRef inst = LLVMGetFirstInstruction(block); inst != NULL; inst = LLVMGetNextInstruction(inst)){
        // if inst is store, add to r_set, remove all stores in r_set that are killed by inst
        if(LLVMIsAStoreInst(inst)){
            vector<LLVMValueRef>* to_del = new vector<LLVMValueRef>();
            for(int i = 0; i < r_set->size(); i++){
                LLVMValueRef curr_inst = r_set->at(i);
                if(LLVMIsAStoreInst(curr_inst) && LLVMGetOperand(curr_inst, 1) == LLVMGetOperand(inst, 1)){
                    to_del->push_back(curr_inst);
        
                }
            }
            r_set->push_back(inst);
            for(int i = 0; i < to_del->size(); i++){
                r_set->erase(std::remove(r_set->begin(), r_set->end(), to_del->at(i)), r_set->end());
            }
        }
        // if inst is a load
        if(LLVMIsALoadInst(inst)){
            // get the address of the load inst -> %t
            LLVMValueRef addr = LLVMGetOperand(inst, 0);
            // find all stores in r_set that writes to addr %t
            vector<LLVMValueRef>* store_inst = find_store_inst(r_set, addr);
            // if there is no store inst that writes to addr, skip
            if(store_inst->size() == 0){
                continue;
            }
            // check if all these store instructions are constant store and store the same constant
            bool all_const = true;
            for(int i = 0; i < store_inst->size(); i++){
                if(!LLVMIsAConstant(LLVMGetOperand(store_inst->at(i), 0))){
                    all_const = false;
                    break;
                }
            }
            bool same = true;
            // REVIEW - right way to get constant ?
            LLVMValueRef constant = LLVMGetOperand(store_inst->at(0), 0);
            if(all_const){
                for(int i = 0; i < store_inst->size(); i++){
                    LLVMValueRef store = store_inst->at(i);
                    // REVIEW - is this the right way to compare constant ?
                    if(LLVMConstIntGetSExtValue(LLVMGetOperand(store, 0)) != LLVMConstIntGetSExtValue(constant)){
                        same = false;
                        break;
                    }
                }
            }
            if(all_const && same){
                LLVMReplaceAllUsesWith(inst, constant);
                marked_load->push_back(inst);
            }
        }
    }
    // delete all the marked load instruction
    for(int i = 0; i < marked_load->size(); i++){
        LLVMInstructionEraseFromParent(marked_load->at(i));
    }

    // check for flag
    if(marked_load->size() == 0){
        return false;
    } else {
        return true;
    }
}

/* helper func: find all store inst in set s that write to a specific address addr */
// REVIEW - check address
vector<LLVMValueRef>* find_store_inst(vector<LLVMValueRef>* s, LLVMValueRef addr){
    vector<LLVMValueRef>* res = new vector<LLVMValueRef>();
    for(int i = 0; i < s->size(); i++){
        LLVMValueRef inst = s->at(i);
        if(LLVMGetInstructionOpcode(inst) == LLVMStore && LLVMGetOperand(inst, 1) == addr){
            res->push_back(inst);
        }
    }
    return res;
}




/* local optimizations */
bool common_subexpression_elimination(LLVMBasicBlockRef bb) {
    // loop over each pair of instructions in the basic block
    LLVMValueRef instruction_1;
    bool flag = false;
    for (instruction_1 = LLVMGetFirstInstruction(bb); instruction_1 != NULL; instruction_1 = LLVMGetNextInstruction(instruction_1)) {
        if(LLVMIsAStoreInst(instruction_1) || LLVMIsACallInst(instruction_1) || LLVMIsAReturnInst(instruction_1) || LLVMIsAAllocaInst(instruction_1)) {
            continue;
        }
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
        return false;
    }else{
        return true;
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

bool dead_code_elimination(LLVMBasicBlockRef bb) {
    bool change = false;
    LLVMValueRef instruction;
    for (instruction = LLVMGetFirstInstruction(bb); instruction != NULL; instruction = LLVMGetNextInstruction(instruction)) {
        // check if the instruction is in the list of instructions to keep
        if(LLVMIsAAllocaInst(instruction) || LLVMIsAReturnInst(instruction) || LLVMIsACallInst(instruction) || LLVMIsAStoreInst(instruction)) {
            continue;
        }

        // check if the instruction is used by any other instruction
        if(LLVMGetFirstUse(instruction) == NULL) {
            // if not, remove the instruction
            LLVMInstructionEraseFromParent(instruction);
            change = true;
        }
    }
    // check for global flag
    if(!change){
        return false;
    } else{
        return true;
    }
}

bool constant_folding(LLVMBasicBlockRef bb) {
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
        return false;
    }else{
        return true;
    }
}