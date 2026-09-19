#include "opcode.h"
#include "vu_string.h"
#include "v_unit.h"

const opcode_info_t opcode_info_table[OP_COUNT] = {
#define OPCODE_INFO(name, size_, arg_type_, func)	\
	[OP_##name] = {						\
		.mnemonic = #name,				\
		.size = (size_),				\
		.arg_type = arg_type_,			\
		.handler = (func),				\
	},
    OPCODE_LIST(OPCODE_INFO)
#undef OPCODE_INFO
};

opcode_work_status halt_handler(v_unit* u, decoded_opcode* opcode) {
    return OPW_EXIT_HALT;
}

opcode_work_status nop_handler(v_unit* u, decoded_opcode* opcode) {
    u->ip += opcode_info_table[opcode->opcode].size;
    return OPW_EXIT_NONE;
}

opcode_work_status jmp_handler(v_unit* u, decoded_opcode* opcode) {
    if (u->memory.size <= opcode->operands[0].dw+1) {
        return OPW_EXIT_ILLEGAL_JUMP;
    }
    u->ip = opcode->operands[0].dw;
    return OPW_EXIT_NONE;
}

opcode_work_status jz_handler(v_unit* u, decoded_opcode* opcode) {
    if (u->cmpResult == 0) {
        return jmp_handler(u, opcode);
    }
    return OPW_EXIT_NONE;
}

opcode_work_status jp_handler(v_unit* u, decoded_opcode* opcode) {
    if (u->cmpResult > 0) {
        return jmp_handler(u, opcode);
    }
    return OPW_EXIT_NONE;
}

opcode_work_status jn_handler(v_unit* u, decoded_opcode* opcode) {
    if (u->cmpResult < 0) {
        return jmp_handler(u, opcode);
    }
    return OPW_EXIT_NONE;
}

opcode_work_status cmprr_handler(v_unit* u, decoded_opcode* opcode) {
    int32_t* a = (int*)opcode->operands[0].reg;
    int32_t* b = (int*)opcode->operands[1].reg;
    u->cmpResult = (*a > *b) - (*a < *b);
    u->ip += opcode_info_table[opcode->opcode].size;
    return OPW_EXIT_NONE;
}

opcode_work_status cmprdw_handler(v_unit* u, decoded_opcode* opcode) {
    int32_t*    a = (int*)opcode->operands[0].reg;
    int         b = (int)opcode->operands[1].dw;
    u->cmpResult = (*a > b) - (*a < b);
    u->ip += opcode_info_table[opcode->opcode].size;
    return OPW_EXIT_NONE;
}

opcode_work_status inc_handler(v_unit* u, decoded_opcode* opcode) {
    (*opcode->operands[0].reg)++;
    u->ip += opcode_info_table[opcode->opcode].size;
    return OPW_EXIT_NONE;
}

opcode_work_status dec_handler(v_unit* u, decoded_opcode* opcode) {
    (*opcode->operands[0].reg)--;
    u->ip += opcode_info_table[opcode->opcode].size;
    return OPW_EXIT_NONE;
}

opcode_work_status add_handler(v_unit* u, decoded_opcode* opcode) {
    *opcode->operands[0].reg += *opcode->operands[1].reg;
    u->ip += opcode_info_table[opcode->opcode].size;
    return OPW_EXIT_NONE;
}

opcode_work_status sub_handler(v_unit* u, decoded_opcode* opcode) {
    *opcode->operands[0].reg -= *opcode->operands[1].reg;
    u->ip += opcode_info_table[opcode->opcode].size;
    return OPW_EXIT_NONE;
}

opcode_work_status mul_handler(v_unit* u, decoded_opcode* opcode) {
    *opcode->operands[0].reg *= *opcode->operands[1].reg;
    u->ip += opcode_info_table[opcode->opcode].size;
    return OPW_EXIT_NONE;
}

opcode_work_status div_handler(v_unit* u, decoded_opcode* opcode) {
    *opcode->operands[0].reg /= *opcode->operands[1].reg;
    u->ip += opcode_info_table[opcode->opcode].size;
    return OPW_EXIT_NONE;
}

opcode_work_status mod_handler(v_unit* u, decoded_opcode* opcode) {
    *opcode->operands[0].reg %= *opcode->operands[1].reg;
    u->ip += opcode_info_table[opcode->opcode].size;
    return OPW_EXIT_NONE;
}

opcode_work_status neg_handler(v_unit* u, decoded_opcode* opcode) {
    *opcode->operands[0].reg = -*opcode->operands[0].reg;
    u->ip += opcode_info_table[opcode->opcode].size;
    return OPW_EXIT_NONE;
}

opcode_work_status and_handler(v_unit* u, decoded_opcode* opcode) {
    u->ip += opcode_info_table[opcode->opcode].size;

}

opcode_work_status or_handler(v_unit* u, decoded_opcode* opcode) {
    u->ip += opcode_info_table[opcode->opcode].size;
}

opcode_work_status xor_handler(v_unit* u, decoded_opcode* opcode) {
    u->ip += opcode_info_table[opcode->opcode].size;
}

opcode_work_status not_handler(v_unit* u, decoded_opcode* opcode) {
    u->ip += opcode_info_table[opcode->opcode].size;
}

opcode_work_status shl_handler(v_unit* u, decoded_opcode* opcode) {
    u->ip += opcode_info_table[opcode->opcode].size;
}

opcode_work_status shr_handler(v_unit* u, decoded_opcode* opcode) {
    u->ip += opcode_info_table[opcode->opcode].size;
}

opcode_work_status load8_handler(v_unit* u, decoded_opcode* opcode) { /* load8 reg, reg */
    *opcode->operands[0].reg = 0;
    uint32_t addr = *opcode->operands[1].reg;
    vu_memcpy(opcode->operands[0].reg, u->memory.addr+addr, 1);
    u->ip += opcode_info_table[opcode->opcode].size;
    return OPW_EXIT_NONE;
}

opcode_work_status load16_handler(v_unit* u, decoded_opcode* opcode) {
    *opcode->operands[0].reg = 0;
    uint32_t addr = *opcode->operands[1].reg;
    vu_memcpy(opcode->operands[0].reg, u->memory.addr+addr, 2);
    u->ip += opcode_info_table[opcode->opcode].size;
    return OPW_EXIT_NONE;
}

opcode_work_status load32_handler(v_unit* u, decoded_opcode* opcode) {
    uint32_t addr = *opcode->operands[1].reg;
    vu_memcpy(opcode->operands[0].reg, u->memory.addr+addr, 4);
    u->ip += opcode_info_table[opcode->opcode].size;
    return OPW_EXIT_NONE;
}

opcode_work_status store8_handler(v_unit* u, decoded_opcode* opcode) {
    uint32_t addr = *opcode->operands[0].reg;
    vu_memcpy(u->memory.addr+addr, opcode->operands[1].reg, 1);
    u->ip += opcode_info_table[opcode->opcode].size;
    return OPW_EXIT_NONE;
}

opcode_work_status store16_handler(v_unit* u, decoded_opcode* opcode) {
    uint32_t addr = *opcode->operands[0].reg;
    vu_memcpy(u->memory.addr+addr, opcode->operands[1].reg, 2);
    u->ip += opcode_info_table[opcode->opcode].size;
    return OPW_EXIT_NONE;
}

opcode_work_status store32_handler(v_unit* u, decoded_opcode* opcode) {
    uint32_t addr = *opcode->operands[0].reg;
    vu_memcpy(u->memory.addr+addr, opcode->operands[1].reg, 4);
    u->ip += opcode_info_table[opcode->opcode].size;
    return OPW_EXIT_NONE;
}

opcode_work_status movrr_handler(v_unit* u, decoded_opcode* opcode) {
    *opcode->operands[0].reg = *opcode->operands[1].reg;
    u->ip += opcode_info_table[opcode->opcode].size;
    return OPW_EXIT_NONE;
}

opcode_work_status movrn_handler(v_unit* u, decoded_opcode* opcode) {
    *opcode->operands[0].reg = opcode->operands[1].dw;
    u->ip += opcode_info_table[opcode->opcode].size;
    return OPW_EXIT_NONE;
}

opcode_work_status syscall_handler(v_unit* u, decoded_opcode* opcode) {
    u->ip += opcode_info_table[opcode->opcode].size;
    return OPW_EXIT_NONE;
}