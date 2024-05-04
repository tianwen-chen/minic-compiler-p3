#include "global_optimizer.h"
#include "DArray.h"
#include "local_optimizer.h"

/* Global Variables */
unordered_map<LLVMBasicBlockRef, D_Array*> gen_dict;
unordered_map<LLVMBasicBlockRef, D_Array*> kill_dict;
unordered_map<LLVMBasicBlockRef, D_Array*> in_dict;
unordered_map<LLVMBasicBlockRef, D_Array*> out_dict;
unordered_map<LLVMBasicBlockRef, vector<LLVMBasicBlockRef>*> pred_dict;     // predecessor dict
unordered_map<LLVMBasicBlockRef, int> bb_indices;       // index basic blocks for debug purpose

/* Global Flags */
bool in_out_change = true;
bool remove_load_change = true;
bool common_subexpression_elimination_change = true;
bool dead_code_elimination_change = true;
bool constant_folding_change = true;

/* Function Declarations */
void compute_GEN_and_KILL(LLVMBasicBlockRef block, D_Array* gen, D_Array* kill);
void compute_IN_and_OUT(LLVMModuleRef m);
void constant_propagation(LLVMBasicBlockRef block);
void construct_predecessor_dict(LLVMModuleRef m);
void construct_bb_indices(LLVMModuleRef m);
void print_global_dict(unordered_map<LLVMBasicBlockRef, vector<LLVMBasicBlockRef>*> dict);
D_Array* find_store_inst(LLVMBasicBlockRef block, LLVMValueRef addr);

/* Global Optimizer -- only need to call this in main */
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
            constant_propagation(block);
        }

        // check for fix point: all global flags false
        if(!in_out_change && !remove_load_change && !common_subexpression_elimination_change && !dead_code_elimination_change && !constant_folding_change){
            change = false;
        }
    }
}

/* Global Optimization Routines */

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

void constant_propagation(LLVMBasicBlockRef block){
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

/* Helper Funcs: Global Dictionaries Construction Routines */
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

/* Helper Funcs */
// Print Global Dicts
void print_global_dict(unordered_map<LLVMBasicBlockRef, vector<LLVMBasicBlockRef>*> dict){
    for(auto it = dict.begin(); it != dict.end(); it++){
        printf("Block: %d\n", bb_indices[it->first]);
        printf("Predecessors: ");
        for(int i = 0; i < it->second->size(); i++){
            printf("%d ", bb_indices[it->second->at(i)]);
        }
        printf("\n");
    }
}

// find all store inst that write to a specific address with a basic block
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