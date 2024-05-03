#include<stdio.h>
#include"Core.h"
#include"local_optimizer.h"
#include <stdbool.h>

LLVMBasicBlockRef common_subexpression_elimination(LLVMBasicBlockRef bb) {
    // loop over each pair of instructions in the basic block
    LLVMValueRef instruction_1;
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
                    }
                }
            }
        }
    }
    return bb;

}

/*
    value1 and value2 are load operations from the same address, check if there is store operation to that address between the two
    if there is, return false, else return true 
*/
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

LLVMBasicBlockRef dead_code_elimination(LLVMBasicBlockRef bb) {
    // TODO: complete the list in header file (or macro?)
    LLVMOpcode instructions_to_keep[3] = {LLVMStore, LLVMCall, LLVMRet};
    // loop over each instruction in the basic block and check if the oprand is used by any other instruction
    LLVMValueRef instruction;
    for (instruction = LLVMGetFirstInstruction(bb); instruction != NULL; instruction = LLVMGetNextInstruction(instruction)) {
        // check if the instruction is in the list of instructions to keep
        bool keep = false;
        // FIXME: number of loops (size of the list), or use a hashset?
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
            }
        }
    }
    
    return bb;

}


/*Your constant folding optimization function will go over all the instructions in the given function and 
find instructions where opcode corresponds to arithmetic operations (+, -, *) and all operands are constants. 
For such instructions, your optimization function should replace all uses of the instruction to point to a constant instruction.

Some useful LLVM functions for this optimization are:

LLVMConstAdd
LLVMConstSub
LLVMConstMul
*/
LLVMBasicBlockRef constant_folding(LLVMBasicBlockRef bb) {
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
            }
        }
    }
    return bb;
}