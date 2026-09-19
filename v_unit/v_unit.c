#include "v_unit.h"
#include "opcode.h"
#include <stdlib.h>
#include "vu_string.h"
#include "vu_stdio.h"

static void debug_registers
(
    v_unit* u,
    uint32_t bufferSize,
    char* buffer,
    uint32_t* written
)
{
    if (!buffer || !bufferSize || bufferSize < 38) {
        if (written) *written = 0;
        return;
    }

    uint8_t* p = (uint8_t*)u + u->regs.offset;
    uint32_t bufferIter = 0;
    vu_snprintf(buffer, bufferSize, "REGISTERS:\n");
    bufferIter += 11;

    for (int i = 0; i < REGS_COUNT; i++) {
        int32_t num = *(int32_t*)(p + i * 4);
        uint32_t remaining = bufferSize - bufferIter;
        int res = vu_snprintf
        (
            buffer + bufferIter,
            remaining,
            "R%03d = %08x:%011d\n",
            i, (uint32_t)num, num
        );

        if (res < 0 || res >= remaining) {
            buffer[bufferSize - 1] = 0;
            bufferIter = bufferSize - 1;
            break;
        }
        bufferIter += res;
        if (written) {
            *written += bufferIter;
        }
    }
}

static void debug_memory
(
    v_unit* u,
    uint32_t bufferSize,
    char* buffer,
    uint32_t* written
)
{
    if (!buffer || !bufferSize || bufferSize < 60) {
        if (written) *written = 0;
        return;
    }

    uint8_t* p = u->memory.addr;
    uint32_t bufferIter = 0;
    int byteRemaining = (int)u->memory.size;

    vu_snprintf(buffer, bufferSize, "MEMORY:\n");
    bufferIter += 8;

    for (int i = 0; byteRemaining > 0; i++) {

        uint32_t len = byteRemaining > 16 ? 16 : byteRemaining;
        char byteArray[49];
        byteRemaining -= 16;

        for (int j = 0; j < len; j++) {
            vu_snprintf(byteArray+j*3, 4,"%02x ", p[j+i*16]);
        }
        byteArray[len*2+len-1] = 0;

        uint32_t remaining = bufferSize - bufferIter;
        int res = vu_snprintf
        (
            buffer + bufferIter,
            remaining,
            "%02x: %s\n",
            i, byteArray
        );

        if (res < 0 || res >= remaining) {
            buffer[bufferSize - 1] = 0;
            bufferIter = bufferSize - 1;
            break;
        }
        bufferIter += res;
    }

    if (written) {
        *written += bufferIter;
    }
}

static vu_exit_code step(v_unit* u) {
    decoded_opcode op = {0};

    uint8_t*    iAddr       = u->memory.addr + u->ip;
    uint32_t*   rBaseAddr   = (uint32_t*)u->regs.addr;
    uint8_t     opcode = *(iAddr);
    op.opcode = opcode;
    const opcode_info_t* info = &opcode_info_table[op.opcode];
    switch (info->arg_type) {
        case OP_ARG_NONE:
            break;
        case OP_ARG_R: {
            op.operandCount = 1;
            op.operands[0].type = OPERAND_TYPE_REG;
            op.operands[0].reg = rBaseAddr + *(iAddr + 1);
            break;
        }
        case OP_ARG_RR: {
            op.operandCount = 2;
            op.operands[0].type = OPERAND_TYPE_REG;
            op.operands[1].type = OPERAND_TYPE_REG;
            op.operands[0].reg = rBaseAddr + *(iAddr + 1);
            op.operands[1].reg = rBaseAddr + *(iAddr + 2);
            break;
        }
        case OP_ARG_B: {
            op.operandCount = 1;
            op.operands[0].type = OPERAND_TYPE_BYTE;
            op.operands[0].b = *(iAddr + 1);
            break;
        }
        case OP_ARG_RB: {
            op.operandCount = 2;
            op.operands[0].type = OPERAND_TYPE_REG;
            op.operands[1].type = OPERAND_TYPE_BYTE;
            op.operands[0].reg  = rBaseAddr + *(iAddr + 1);
            op.operands[1].b    = *(iAddr + 2);
            break;
        }
        case OP_ARG_DWORD: {
            op.operandCount = 1;
            op.operands[0].type = OPERAND_TYPE_DWORD;
            vu_memcpy(&op.operands[0].dw, (iAddr + 1), 4);
            break;
        }
        case OP_ARG_DWORD_R: {
            op.operandCount = 2;
            op.operands[0].type = OPERAND_TYPE_DWORD;
            op.operands[1].type = OPERAND_TYPE_REG;
            vu_memcpy(&op.operands[0].dw, (iAddr + 1), 4);
            op.operands[1].reg = rBaseAddr + *(iAddr + 5);
            break;
        }
        case OP_ARG_R_DWORD: {
            op.operandCount = 2;
            op.operands[0].type = OPERAND_TYPE_REG;
            op.operands[1].type = OPERAND_TYPE_DWORD;
            op.operands[0].reg = rBaseAddr + *(iAddr + 1);
            vu_memcpy(&op.operands[1].dw, (iAddr + 2), 4);
            break;
        }
        default: {
            /* в будующем сделать обработчик ошибок */
            break;
        }
    }

    opcode_handler_t handler = info->handler;
    if (handler) {
        u->cause_id = handler(u, &op);
    }

    return VU_EXIT_NONE;
}



static struct vu_ops ops = {
    step, 0, 0, debug_registers, debug_memory, 0
};

vu_create_status v_unit_new(
    uint32_t memory_size,
    v_unit** unit
) {
    if (!unit) {
        return VU_NULL_PARAM;
    }
    uint32_t memSize = sizeof(v_unit);
    memSize += REGS_COUNT*sizeof(uint32_t);
    memSize += memory_size;
    memSize += sizeof(uint32_t);
    v_unit* ret = malloc(memSize);
    if (!ret) return VU_CREATE_ALLOCATION_FAILED;
    *unit = ret;
    vu_memset(ret, 0, memSize);

    ret->ops = &ops;

    ret->regs.offset        = (int)sizeof(v_unit);
    ret->regs.size          = REGS_COUNT*sizeof(int32_t);
    ret->regs.addr           = (uint8_t*)ret + ret->regs.offset;

    ret->memory.offset     = ret->regs.offset + ret->regs.size;
    ret->memory.size       = memory_size;
    ret->memory.addr       = (uint8_t*)ret + ret->memory.offset;

    return VU_CREATE_OK;
}
