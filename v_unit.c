//
// Created by xinitrix on 04.09.2026.
//

#include "v_unit.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


static char nibble_to_ascii(char nibble) {
    if (nibble >= 0 && nibble <= 9) {
        return nibble + '0';
    }
    if (nibble > 9 && nibble <= 15) {
        return nibble-10 + 'A';
    }
    return 0;
}

static int find_var2
(
    v_unit* u,
    const char* key1,
    const char* key2,
    int32_t** val1,
    int32_t** val2
)
{
    if (!key1 || !key2) return -1;
    int f1 = 0;
    int f2 = 0;
    for (int i = 0; i < u->vars.key_count; i+=1) {
        char key[7] = {0};
        memcpy(key, u->vars.key + i*6, 6);
        int res1 = strcmp(key1, key);
        int res2 = strcmp(key2, key);
        if (res1 == 0) {
            f1 = 1;
            *val1 = &u->vars.value[i];
        }
        if (res2 == 0) {
            f2 = 1;
            *val2 = &u->vars.value[i];
        }
    }
    if (f1 && f2 != 1) {
        return -1;
    }
    return 1;
}

static int find_var1
(
    v_unit* u,
    const char* key1,
    int32_t** val1
)
{
    if (!key1) return -1;
    int f1 = 0;
    for (int i = 0; i < u->vars.key_count; i+=1) {
        char key[7] = {0};
        memcpy(key, u->vars.key + i*6, 6);
        int res1 = strcmp(key1, key);
        if (res1 == 0) {
            f1 = 1;
            *val1 = &u->vars.value[i];
        }
    }
    if (f1 != 1) return -1;
    return 1;
}

static int new_var(v_unit* u, const char* name, int32_t val) {
    if (!name || !u) return -1;
    memcpy(u->vars.key+u->vars.key_count*6, name, 6);
    u->vars.value[u->vars.key_count] = val;
    u->vars.key_count+=1;
    return 1;
}

static void reg_dump(v_unit* u) {
    uint8_t* p = (uint8_t*)u;
    FILE* f = fopen("reg.txt", "w");
    for (int i = 0; i < REGS_COUNT; i+=1) {
        int num = *(int32_t*)(p+u->regs.offset+i*4);
        for (int j = 0; j < 4; j+=1) {
            uint8_t* reg = (uint8_t*)(p+u->regs.offset+i*4+j);
            char lowNibble = *(reg) & 0x0F;
            char highNibble = (*(reg) & 0xFF) >> 4;
            lowNibble = nibble_to_ascii(lowNibble);
            highNibble = nibble_to_ascii(highNibble);
            fprintf(f, "%c", highNibble);
            fprintf(f, "%c", lowNibble);
        }
        fprintf(f, ":%d ", num);
    }
    fclose(f);
}

static void var_dump(v_unit* u) {
    uint8_t* p = (uint8_t*)u;
    FILE* f = fopen("vars.txt", "w");
    for (int i = 0; i < u->vars.key_count; i+=1) {
        char key[7] = {0};
        uint8_t* kPtr = (uint8_t*)u->vars.key;
        memcpy(key, kPtr+i*6, 6);
        int32_t num = u->vars.value[i];

        uint8_t* numPtr = (uint8_t*)&num;
        fprintf(f, ":%s(", key);
        for (int j = 0; j < 4; j+=1) {
            char lowNibble = *(numPtr+j) & 0x0F;
            char highNibble = (*(numPtr+j) & 0xFF) >> 4;
            lowNibble = nibble_to_ascii(lowNibble);
            highNibble = nibble_to_ascii(highNibble);
            fprintf(f, "%c", highNibble);
            fprintf(f, "%c", lowNibble);
        }
        fprintf(f, "):%d ", num);
    }
    fclose(f);
}

// -2 выполнение экстренно завершено
// -1 был пропуск инструкции
static int step(v_unit* u) {
    uint8_t*    uPtr;
    int32_t*   regs_addr;
    uint8_t*    prog_addr;
    uint8_t*    window_args_addr;
    uint8_t*    window_data_addr;
    uint8_t*    data_addr;
    uPtr                = (uint8_t*)u;
    regs_addr           = (int32_t*)(uPtr + u->regs.offset);
    prog_addr           = uPtr + u->program.offset;
    window_args_addr    = uPtr + u->window_args.offset;
    window_data_addr    = uPtr + u->window_data.offset;
    data_addr           = uPtr + u->window_data.offset;
    int op = *(prog_addr + u->pc);
    switch (op) {
        case OP_HALT: {
            u->exitCode = VU_EXIT_HALT;
            return 0;
        }
        case OP_NOP: {
            u->pc += 1;
            return 1;
        }
        case OP_JMP: {
            int addr = 0;
            memcpy(&addr, prog_addr+u->pc+1, 4);
            if (addr >= u->program.size) {
                if (!u->fault_mode) { /*fault == 0 */
                    u->fault_addr = u->pc;
                    u->exitCode = VU_EXIT_ILLEGAL_JUMP;
                    return -2;
                }
                return -1;
            }
            u->pc = addr;
            return 1;
        }

        case OP_JZ: {
            if (u->cmpResult != 0)
                return 1;

            int addr = 0;
            memcpy(&addr, prog_addr+u->pc+1, 4);
            if (addr >= u->program.size) {
                if (!u->fault_mode) { /*fault == 0 */
                    u->fault_addr = u->pc;
                    u->exitCode = VU_EXIT_ILLEGAL_JUMP;
                    return -2;
                }
                return -1;
            }
            u->pc = addr;
            return 1;
        }

        case OP_JP: {
            if (!u->cmpResult > 0)
                return 1;

            int addr = 0;
            memcpy(&addr, prog_addr+u->pc+1, 4);
            if (addr >= u->program.size) {
                if (!u->fault_mode) { /*fault == 0 */
                    u->fault_addr = u->pc;
                    u->exitCode = VU_EXIT_ILLEGAL_JUMP;
                    return -2;
                }
                return -1;
            }
            u->pc = addr;
            return 1;
        }

        case OP_JN: {
            if (!u->cmpResult < 0)
                return 1;

            int addr = 0;
            memcpy(&addr, prog_addr+u->pc+1, 4);
            if (addr >= u->program.size) {
                if (!u->fault_mode) { /*fault == 0 */
                    u->fault_addr = u->pc;
                    u->exitCode = VU_EXIT_ILLEGAL_JUMP;
                    return -2;
                }
                return -1;
            }
            u->pc = addr;
            return 1;
        }

        case OP_CMPRR: { // cmprr 4 4
            int reg1Index = *(prog_addr+u->pc+1);
            int reg2Index = *(prog_addr+u->pc+2);
            int a = *(regs_addr+reg1Index);
            int b = *(regs_addr+reg2Index);
            u->cmpResult = (int8_t)((a > b) - (a < b));
            u->pc += 3;
            return 1;
        }

        case OP_CMPVV: { //cmpvv mnemonic1 mnemonic2
            int* a = NULL;
            int* b = NULL;
            char mn1[7] = {0};
            char mn2[7] = {0};
            memcpy(mn1, prog_addr+u->pc+1, 6);
            memcpy(mn2, prog_addr+u->pc+7, 6);

            //поиск числовых значений
            find_var2(u, mn1, mn2, &a, &b);
            u->cmpResult = (int8_t)((*a > *b) - (*a < *b));
            u->pc += 13;
            return 1;
        }

        case OP_CMPRV: { // cmprv reg(1) var(6)
            int regIndex = *(prog_addr+u->pc+1);
            int a = *(regs_addr+regIndex);
            char mn1[7] = {0};
            memcpy(mn1, prog_addr+u->pc+2, 6);
            int* b = NULL;
            find_var1(u, mn1, &b);
            u->cmpResult = (int8_t)((a > *b) - (a < *b));
            u->pc += 8;
            return 1;
        }
        case OP_CMPVR: { // cmpvr var(6) reg(1)
            char mn1[7] = {0};
            memcpy(mn1, prog_addr+u->pc+1, 6);
            int* a = NULL;
            find_var1(u, mn1, &a);

            int regIndex = *(prog_addr+u->pc+7);
            int b = *(regs_addr+regIndex);

            u->cmpResult = (int8_t)((*a > b) - (*a < b));
            u->pc += 8;
            return 1;
        }

        case OP_ADD_R: {
            int r1 = *(prog_addr+u->pc+1);
            int r2 = *(prog_addr+u->pc+2);
            int32_t* op1 = regs_addr + r1;
            int32_t* op2 = regs_addr + r2;
            *op1 += *op2;
            u->pc += 3;
            return 1;
        }
        case OP_SUB_R: {
            int r1 = *(prog_addr+u->pc+1);
            int r2 = *(prog_addr+u->pc+2);
            int32_t* op1 = regs_addr + r1;
            int32_t* op2 = regs_addr + r2;
            *op1 -= *op2;
            u->pc += 3;
            return 1;
        }

        case OP_MUL_R: {
            int r1 = *(prog_addr+u->pc+1);
            int r2 = *(prog_addr+u->pc+2);
            int32_t* op1 = regs_addr + r1;
            int32_t* op2 = regs_addr + r2;
            *op1 *= *op2;
            u->pc += 3;
            return 1;
        }

        case OP_DIV_R: {
            int r1 = *(prog_addr+u->pc+1);
            int r2 = *(prog_addr+u->pc+2);
            int32_t* op1 = regs_addr + r1;
            int32_t* op2 = regs_addr + r2;
            *op1 /= *op2;
            u->pc += 3;
            return 1;
        }

        case OP_ADD_V: { // addv v1(6) v2(6)
            int* v1 = 0;
            int* v2 = 0;
            char mn1[7] = {0};
            char mn2[7] = {0};
            memcpy(mn1, prog_addr+u->pc+1, 6);
            memcpy(mn2, prog_addr+u->pc+7, 6);

            //поиск числовых значений
            find_var2(u, mn1, mn2, &v1, &v2);
            *v1 += *v2;
            u->pc += 13;
            return 1;
        }
        case OP_SUB_V: {
            int* v1 = 0;
            int* v2 = 0;
            char mn1[7] = {0};
            char mn2[7] = {0};
            memcpy(mn1, prog_addr+u->pc+1, 6);
            memcpy(mn2, prog_addr+u->pc+7, 6);

            //поиск числовых значений
            find_var2(u, mn1, mn2, &v1, &v2);
            *v1 -= *v2;
            u->pc += 13;
            return 1;
        }

        case OP_MUL_V: {
            int* v1 = 0;
            int* v2 = 0;
            char mn1[7] = {0};
            char mn2[7] = {0};
            memcpy(mn1, prog_addr+u->pc+1, 6);
            memcpy(mn2, prog_addr+u->pc+7, 6);

            //поиск числовых значений
            find_var2(u, mn1, mn2, &v1, &v2);
            *v1 *= *v2;
            u->pc += 13;
            return 1;
        }

        case OP_DIV_V: {
            int* v1 = 0;
            int* v2 = 0;
            char mn1[7] = {0};
            char mn2[7] = {0};
            memcpy(mn1, prog_addr+u->pc+1, 6);
            memcpy(mn2, prog_addr+u->pc+7, 6);

            //поиск числовых значений
            find_var2(u, mn1, mn2, &v1, &v2);
            *v1 /= *v2;
            u->pc += 13;
            return 1;
        }
        case OP_MOD_V: {
            int* v1 = 0;
            int* v2 = 0;
            char mn1[7] = {0};
            char mn2[7] = {0};
            memcpy(mn1, prog_addr+u->pc+1, 6);
            memcpy(mn2, prog_addr+u->pc+7, 6);

            //поиск числовых значений
            find_var2(u, mn1, mn2, &v1, &v2);
            *v1 %= *v2;
            u->pc += 13;
            return 1;
        }
        case OP_NEG_V: { // negv var(6)
            int* v1 = 0;
            char mn1[7] = {0};
            memcpy(mn1, prog_addr+u->pc+1, 6);

            //поиск числовых значений
            find_var1(u, mn1, &v1);
            *v1 = ~*v1;
            u->pc += 7;
            return 1;
        }


        case OP_ADD_VR: { // addv varVal(6) regVal2(1)
            int* varVal = 0;
            int* regVal = 0;

            char mn1[7] = {0};
            memcpy(mn1, prog_addr+u->pc+1, 6);
            find_var1(u, mn1, &varVal);

            int regIndex = *(prog_addr+u->pc+7);
            regVal = regs_addr+regIndex;

            *varVal += *regVal;
            u->pc += 8;
            return 1;
        }
        case OP_SUB_VR: {
            int* varVal = 0;
            int* regVal = 0;

            char mn1[7] = {0};
            memcpy(mn1, prog_addr+u->pc+1, 6);
            find_var1(u, mn1, &varVal);

            int regIndex = *(prog_addr+u->pc+7);
            regVal = regs_addr+regIndex;

            *varVal -= *regVal;
            u->pc += 8;
            return 1;
        }

        case OP_MUL_VR: {
            int* varVal = 0;
            int* regVal = 0;

            char mn1[7] = {0};
            memcpy(mn1, prog_addr+u->pc+1, 6);
            find_var1(u, mn1, &varVal);

            int regIndex = *(prog_addr+u->pc+7);
            regVal = regs_addr+regIndex;

            *varVal -= *regVal;
            u->pc *= 8;
            return 1;
        }

        case OP_DIV_VR: {
            int* varVal = 0;
            int* regVal = 0;

            char mn1[7] = {0};
            memcpy(mn1, prog_addr+u->pc+1, 6);
            find_var1(u, mn1, &varVal);

            int regIndex = *(prog_addr+u->pc+7);
            regVal = regs_addr+regIndex;

            *varVal -= *regVal;
            u->pc /= 8;
            return 1;
        }

        case OP_ADD_RV: { // addv regVal(1) varVal2(6)
            int* regVal = 0;
            int* varVal = 0;

            int regIndex = *(prog_addr+u->pc+1);
            regVal = regs_addr+regIndex;

            char mn1[7] = {0};
            memcpy(mn1, prog_addr+u->pc+2, 6);
            find_var1(u, mn1, &varVal);

            *regVal += *varVal;
            u->pc += 8;
            return 1;
        }
        case OP_SUB_RV: {
            int* regVal = 0;
            int* varVal = 0;

            int regIndex = *(prog_addr+u->pc+1);
            regVal = regs_addr+regIndex;

            char mn1[7] = {0};
            memcpy(mn1, prog_addr+u->pc+2, 6);
            find_var1(u, mn1, &varVal);

            *regVal -= *varVal;
            u->pc += 8;
            return 1;
        }

        case OP_MUL_RV: {
            int* regVal = 0;
            int* varVal = 0;

            int regIndex = *(prog_addr+u->pc+1);
            regVal = regs_addr+regIndex;

            char mn1[7] = {0};
            memcpy(mn1, prog_addr+u->pc+2, 6);
            find_var1(u, mn1, &varVal);

            *regVal *= *varVal;
            u->pc += 8;
            return 1;
        }

        case OP_DIV_RV: {
            int* regVal = 0;
            int* varVal = 0;

            int regIndex = *(prog_addr+u->pc+1);
            regVal = regs_addr+regIndex;

            char mn1[7] = {0};
            memcpy(mn1, prog_addr+u->pc+2, 6);
            find_var1(u, mn1, &varVal);

            *regVal /= *varVal;
            u->pc += 8;
            return 1;
        }
        case OP_MOD_RV: {
            int* regVal = 0;
            int* varVal = 0;

            int regIndex = *(prog_addr+u->pc+1);
            regVal = regs_addr+regIndex;

            char mn1[7] = {0};
            memcpy(mn1, prog_addr+u->pc+2, 6);
            find_var1(u, mn1, &varVal);

            *regVal %= *varVal;
            u->pc += 8;
            return 1;
        }

        case OP_LOAD8: { // load8 reg(1) addr(4)
            int regIndex = *(prog_addr+u->pc+1);
            uint32_t addr;
            memcpy(&addr, prog_addr+u->pc+2, 4);
            int32_t* reg1 = regs_addr + regIndex;
            *reg1 = 0;
            memcpy(reg1, data_addr+addr, 1);
            u->pc += 6;
            return 1;
        }
        case OP_LOAD16: {
            int regIndex = *(prog_addr+u->pc+1);
            uint32_t addr;
            memcpy(&addr, prog_addr+u->pc+2, 4);
            int32_t* reg1 = regs_addr + regIndex;
            *reg1 = 0;
            memcpy(reg1, data_addr+addr, 2);
            u->pc += 6;
            return 1;
        }
        case OP_LOAD32: {
            int regIndex = *(prog_addr+u->pc+1);
            uint32_t addr;
            memcpy(&addr, prog_addr+u->pc+2, 4);
            int32_t* reg1 = regs_addr + regIndex;
            memcpy(reg1, data_addr+addr, 4);
            u->pc += 6;
            return 1;
        }
        case OP_STORE8: { // store8 addr reg
            uint32_t addr;
            memcpy(&addr, prog_addr+u->pc+1, 4);
            int regIndex1 = *(prog_addr+u->pc+5);
            int32_t* reg1 = regs_addr + regIndex1;
            memcpy(data_addr+addr, reg1, 1);
            u->pc += 6;
            return 1;
        }
        case OP_STORE16: {
            uint32_t addr;
            memcpy(&addr, prog_addr+u->pc+1, 4);
            int regIndex1 = *(prog_addr+u->pc+5);
            int32_t* reg1 = regs_addr + regIndex1;
            memcpy(data_addr+addr, reg1, 2);
            u->pc += 6;
            return 1;
        }
        case OP_STORE32: {
            uint32_t addr;
            memcpy(&addr, prog_addr+u->pc+1, 4);
            int regIndex1 = *(prog_addr+u->pc+5);
            int32_t* reg1 = regs_addr + regIndex1;
            memcpy(data_addr+addr, reg1, 2);
            u->pc += 6;
            return 1;
        }

        case OP_VTOR: { // vtor r, v
            int* regPtr = 0;
            int* varPtr = 0;

            int regIndex = *(prog_addr+u->pc+1);
            regPtr = regs_addr+regIndex;

            char mn1[7] = {0};
            memcpy(mn1, prog_addr+u->pc+2, 6);
            find_var1(u, mn1, &varPtr);

            *regPtr = *varPtr;
            u->pc += 8;
            return 1;
        }
        case OP_RTOV: { // rtov v, r
            int* regPtr = 0;
            int* varPtr = 0;

            char mn1[7] = {0};
            memcpy(mn1, prog_addr+u->pc+1, 6);
            find_var1(u, mn1, &varPtr);

            int regIndex = *(prog_addr+u->pc+7);
            regPtr = regs_addr+regIndex;

            *varPtr = *regPtr;
            u->pc += 8;
            return 1;
        }
        case OP_SETR: { //setr reg(1) val(4)
            int regIndex = *(prog_addr+u->pc+1);
            int* regVal = regs_addr+regIndex;
            memcpy(regVal, prog_addr+u->pc+2, 4);
            printf("%x\n", *regVal);
            u->pc += 6;
            return 1;
        }
        case OP_SETV: { //setv var(6) val(4)
            int* varVal = 0;
            char mn1[7] = {0};
            memcpy(mn1, prog_addr+u->pc+1, 6);
            find_var1(u, mn1, &varVal);

            memcpy(varVal, prog_addr+u->pc+7, 4);
            u->pc += 11;
            return 1;
        }
        case OP_NEWVAR: { //newvar var(6)
            char mn1[7] = {0};
            memcpy(mn1, prog_addr+u->pc+1, 6);
            new_var(u, mn1, 0);
            u->pc += 7;
            return 1;
        }
        case OP_DBGREG: {
            u->pc += 1;
            reg_dump(u);
            return 1;
        }
        case OP_DBGVAR: {
            u->pc += 1;
            var_dump(u);
            return 1;
        }
    }

    return -3;
}

static int execute(v_unit* u, uint32_t max_steps) {
    int result = -4; //код даже не начинал выполняться
    for (int i = 0; i < max_steps; i++) {
        result = step(u);
        if (result < 0) {
            return result;
        }
    }
    return result;
}
static void reset(v_unit* u) {
    int uSize = sizeof(v_unit);
    uSize += (int)u->regs.size*(int)sizeof(int32_t);
    uSize += u->program.size;
    uSize += u->window_args.size;
    uSize += u->window_data.size;
    uSize += u->data.size;
    memset(u, 0, uSize);
}

static struct vu_ops ops = {
    step,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0 ,
    0,
    0,
    0,
    0
};

v_unit* v_unit_new(
    int data_size,
    int prog_size
) {
    if (data_size*1024 <= WINDOW_BASE_SIZE+FREE_MEMORY_MIN) return NULL;
    int memSize = sizeof(v_unit);
    memSize += REGS_COUNT*sizeof(int32_t);
    memSize += data_size*1024;
    memSize += prog_size;
    memSize += sizeof(uint32_t);
    memSize += sizeof(vu_arg_header);
    memSize += WINDOW_BASE_SIZE-sizeof(vu_arg_header);
    v_unit* ret = malloc(memSize);
    memset(ret, 0, memSize);

    ret->ops = &ops;

    ret->regs.offset        = (int)sizeof(v_unit);
    ret->regs.size          = REGS_COUNT*sizeof(int32_t);

    ret->program.offset     = ret->regs.offset + ret->regs.size;
    ret->program.size       = prog_size;

    ret->window_args.offset = ret->program.offset+prog_size;
    ret->window_args.size   = sizeof(vu_arg_header);

    ret->window_data.offset = ret->window_args.offset+sizeof(vu_arg_header);
    ret->window_data.size   = WINDOW_BASE_SIZE-sizeof(vu_arg_header);

    ret->data.offset        = ret->window_data.offset+ret->window_data.size;
    ret->data.size          = data_size*1024;

    return ret;
}
