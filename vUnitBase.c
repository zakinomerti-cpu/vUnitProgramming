//
// Created by xinitrix on 04.09.2026.
//

#include "vUnitBase.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static int execute(vUnitBase* vUnitB) {
    uint8_t* pUB = (uint8_t*)vUnitB;
    uint8_t* Current_memAddr = pUB + vUnitB->memory_offset + vUnitB->pc;
    int32_t* reg0Addr = (int32_t*)(pUB + vUnitB->regs_offset); // regs are int32_t
    int inst = *Current_memAddr;
    switch (inst) {
        case OP_HALT:
            vUnitB->halted = 1;
            return 0;
        case OP_ADD: {
            int r1 = *(Current_memAddr+1);
            int r2 = *(Current_memAddr+2);
            int32_t* op1 = reg0Addr + r1;
            int32_t* op2 = reg0Addr + r2;
            *op1 += *op2;
            vUnitB->pc += 3;
            return 1;
        }
        case OP_SUB: {
            int r1 = *(Current_memAddr+1);
            int r2 = *(Current_memAddr+2);
            int32_t* op1 = reg0Addr + r1;
            int32_t* op2 = reg0Addr + r2;
            *op1 -= *op2;
            vUnitB->pc += 3;
            return 1;
        }

        case OP_MUL: {
            int r1 = *(Current_memAddr+1);
            int r2 = *(Current_memAddr+2);
            int32_t* op1 = reg0Addr + r1;
            int32_t* op2 = reg0Addr + r2;
            *op1 *= *op2;
            vUnitB->pc += 3;
            return 1;
        }

        case OP_DIV: {
            int r1 = *(Current_memAddr+1);
            int r2 = *(Current_memAddr+2);
            int32_t* op1 = reg0Addr + r1;
            int32_t* op2 = reg0Addr + r2;
            *op1 /= *op2;
            vUnitB->pc += 3;
            return 1;
        }

        case OP_NEG: {
            int regIndex = *(Current_memAddr+1);
            int32_t* reg1 = reg0Addr + regIndex;
            *reg1 = -*reg1;
            vUnitB->pc += 2;
            return 1;
        }
        case OP_NOT: {
            int regIndex = *(Current_memAddr+1);
            int32_t* reg1 = reg0Addr + regIndex;
            *reg1 = ~*reg1;
            vUnitB->pc += 2;
            return 1;
        }
        case OP_JMP: {
            memcpy(&vUnitB->pc, Current_memAddr+1, 4);
            return 1;
        }
        case OP_CMP: {
            int regOutIndex = *(Current_memAddr+1);
            int reg1Index = *(Current_memAddr+2);
            int reg2Index = *(Current_memAddr+3);
            int a = *(reg0Addr+reg1Index);
            int b = *(reg0Addr+reg2Index);
            *(reg0Addr+regOutIndex) = (a > b) - (a < b);
            vUnitB->pc += 4;
            return 1;
        }
        case OP_PUSH: {
            int32_t value = 0;
            uint32_t op1 = *(Current_memAddr+1);
            if (op1 > 4) return -1;
            memcpy(&value, Current_memAddr+2, op1);
            *(pUB + vUnitB->stack_offset + vUnitB->sp) = value;
            vUnitB->pc += op1+2;
            vUnitB->sp += 1;
            return 1;
        }
        case OP_POP: {
            int regOutIndex = *(Current_memAddr+1);
            vUnitB->sp -= 1;
            *(reg0Addr+regOutIndex) = *(pUB + vUnitB->stack_offset + vUnitB->sp);
            vUnitB->pc += 2;
            return 1;
        }
        case OP_LOAD_8: {
            int regIndex = *(Current_memAddr+1);
            uint32_t addr;
            memcpy(&addr, Current_memAddr+2, 4);
            int32_t* reg1 = reg0Addr + regIndex;
            *reg1 = 0;
            memcpy(reg1, Current_memAddr+addr, 1);
            vUnitB->pc += 6;
            return 1;
        }
        case OP_LOAD_16: {
            int regIndex = *(Current_memAddr+1);
            uint32_t addr;
            memcpy(&addr, Current_memAddr+2, 4);
            int32_t* reg1 = reg0Addr + regIndex;
            *reg1 = 0;
            memcpy(reg1, Current_memAddr+addr, 2);
            vUnitB->pc += 6;
            return 1;
        }
        case OP_LOAD_32: {
            int regIndex = *(Current_memAddr+1);
            uint32_t addr;
            memcpy(&addr, Current_memAddr+2, 4);
            int32_t* reg1 = reg0Addr + regIndex;
            memcpy(reg1, Current_memAddr+addr, 4);
            vUnitB->pc += 6;
            return 1;
        }
        case OP_STORE_8: {
            uint32_t addr;
            memcpy(&addr, Current_memAddr+1, 4);
            int regIndex1 = *(Current_memAddr+5);
            int32_t* reg1 = reg0Addr + regIndex1;
            memcpy(Current_memAddr+addr, reg1, 1);
            vUnitB->pc += 6;
            return 1;
        }
        case OP_STORE_16: {
            uint32_t addr;
            memcpy(&addr, Current_memAddr+1, 4);
            int regIndex1 = *(Current_memAddr+5);
            int32_t* reg1 = reg0Addr + regIndex1;
            memcpy(Current_memAddr+addr, reg1, 2);
            vUnitB->pc += 6;
            return 1;
        }
        case OP_STORE_32: {
            uint32_t addr;
            memcpy(&addr, Current_memAddr+1, 4);
            int regIndex1 = *(Current_memAddr+5);
            int32_t* reg1 = reg0Addr + regIndex1;
            memcpy(Current_memAddr+addr, reg1, 4);
            vUnitB->pc += 6;
            return 1;
        }
        default:
            return -1;
    }
    return -1;
}

static void pushByte(vUnitBase* vCpu, unsigned char byte) {
    uint8_t* p = (uint8_t*)vCpu;
    *(p+vCpu->memory_offset+vCpu->mp) = byte;
    vCpu->mp+=1;
}

static void pushData(vUnitBase* vCpu, void* data, int size) {
    unsigned char* dataPtr = (unsigned char*)data;
    for (int i = 0; i < size; i+=1) {
        pushByte(vCpu, dataPtr[i]);
    }
}

static void setByte(vUnitBase* vCpu, unsigned char byte, int index) {
    uint8_t* p = (uint8_t*)vCpu;
    *(p+vCpu->memory_offset+index) = byte;
}

static void setData(vUnitBase* vCpu, void* data, int size, int index) {
    uint8_t* dataPtr = (uint8_t*)data;
    for (int i = 0; i < size; i+=1) {
        setByte(vCpu, dataPtr[i], index);
        index += 1;
    }
}

static char nibble_to_ascii(char nibble) {
    if (nibble >= 0 && nibble <= 9) {
        return nibble + '0';
    }
    if (nibble > 9 && nibble <= 15) {
        return nibble-10 + 'A';
    }
    return 0;
}

static int32_t getRegValue(vUnitBase* vCpu, unsigned char reg) {
    if (reg >= 0 && reg <= vCpu->regs_count) {
        uint8_t* p = (uint8_t*)vCpu;
        p += vCpu->regs_offset;
        int32_t* r = (int32_t*)p + reg;
        return *r;
    }
    return -1;
}

static void memory_dump(vUnitBase* vCpu) {
    uint8_t* p = (uint8_t*)vCpu;
    FILE* f = fopen("memory.txt", "w");
    for (int i = 0; i < vCpu->memory_size; i+=1) {
        char lowNibble = *(p+vCpu->memory_offset+i) & 0x0F;
        char highNibble = (*(p+vCpu->memory_offset+i) & 0xFF) >> 4;
        lowNibble = nibble_to_ascii(lowNibble);
        highNibble = nibble_to_ascii(highNibble);
        fprintf(f, "%c", highNibble);
        fprintf(f, "%c", lowNibble);
        if (i % 16 == 0 && i != 0) {
            fprintf(f, "\n%x\t\t", i);
        }
        else {
            fprintf(f, " ");
        }
    }
    fclose(f);
}

static const vUnitBaseInterface ops = {
    execute,
    getRegValue,
    pushByte,
    pushData,
    setByte,
    setData,
    memory_dump
};

vUnitBase* vUnitBase_new(
    int numOfRegs,
    int sizeOfStack,
    int sizeOfMemory
) {
    int memSize = sizeof(vUnitBase);
    memSize += numOfRegs*(int)sizeof(int32_t);
    memSize += sizeOfStack;
    memSize += sizeOfMemory;
    vUnitBase* ret = malloc(memSize);
    memset(ret, 0, memSize);
    ret->ops = &ops;
    ret->regs_count = numOfRegs;
    ret->stack_size = sizeOfStack;
    ret->memory_size = sizeOfMemory;

    ret->regs_offset = sizeof(vUnitBase);
    ret->stack_offset = ret->regs_offset;
    ret->stack_offset += sizeof(int32_t)*numOfRegs;

    ret->memory_offset = ret->stack_offset;
    ret->memory_offset += sizeOfStack;
    return ret;
}
