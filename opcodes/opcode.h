#ifndef UNTITLED_OPCODE_H
#define UNTITLED_OPCODE_H
#include <stdint.h>

struct v_unit;

typedef enum {
	OPW_EXIT_NONE = 0,
	OPW_EXIT_HALT,
	OPW_EXIT_INVALID_OPCODE,
	OPW_EXIT_INVALID_OPERAND,
	OPW_EXIT_BAD_REGISTER,
	OPW_EXIT_MEMORY_READ,
	OPW_EXIT_MEMORY_WRITE,
	OPW_EXIT_MEMORY_EXEC,
	OPW_EXIT_DIVIDE_BY_ZERO,
	OPW_EXIT_ILLEGAL_JUMP,
	OPW_EXIT_INTERNAL_ERROR,
} opcode_work_status;

typedef enum {
	OP_ARG_NONE = 0,
	OP_ARG_R,
	OP_ARG_RR,
	OP_ARG_B,
	OP_ARG_RB,
	OP_ARG_DWORD,
	OP_ARG_R_DWORD,
	OP_ARG_DWORD_R,
} OP_ARG_TYPE;

typedef enum {
	OPERAND_TYPE_NONE = 0,
	OPERAND_TYPE_BYTE,
	OPERAND_TYPE_WORD,
	OPERAND_TYPE_DWORD,
	OPERAND_TYPE_REG,
} OPERAND_TYPE;

typedef struct {
	char type;
	union {
		uint32_t* reg;
		uint8_t b;
		uint32_t dw;
	};
} operand_t;

typedef struct {
	uint8_t opcode;
	int operandCount;
	operand_t operands[3];
} decoded_opcode;

typedef opcode_work_status (*opcode_handler_t)(struct v_unit* u, decoded_opcode* opcode);

typedef struct opcode_info {
	char mnemonic[16];
	uint8_t size;
	OP_ARG_TYPE arg_type;
	opcode_handler_t handler;
} opcode_info_t;

/* добавлять отпкоды тут! */
#define OPCODE_LIST(X)																	\
	X(HALT,		1, OP_ARG_NONE,		halt_handler)										\
	X(NOP,		1, OP_ARG_NONE,		nop_handler)											\
	X(JMP,		5, OP_ARG_DWORD,	jmp_handler)											\
	X(JZ,		5, OP_ARG_DWORD,	jz_handler)												\
	X(JP,		5, OP_ARG_DWORD,	jp_handler)												\
	X(JN,		5, OP_ARG_DWORD,	jn_handler)												\
	X(CMPRR,	3, OP_ARG_RR,		cmprr_handler)	/* cmp reg1(1), reg2(1), */				\
	X(CMPRDW,	6, OP_ARG_R_DWORD,	cmprdw_handler)	/* cmp reg1(1), number(4) */			\
																						\
	/* арифметика */																	\
	X(INC,		2, OP_ARG_R,		inc_handler)											\
	X(DEC,		2, OP_ARG_R,		dec_handler)											\
	X(ADD,		3, OP_ARG_RR,		add_handler)											\
	X(SUB,		3, OP_ARG_RR,		sub_handler)											\
	X(MUL,		3, OP_ARG_RR,		mul_handler)											\
	X(DIV,		3, OP_ARG_RR,		div_handler)											\
	X(MOD,		3, OP_ARG_RR,		mod_handler)											\
	X(NEG,		2, OP_ARG_R,		neg_handler)											\
																						\
	/* битовые операции, только регистры */												\
	X(AND,		1, OP_ARG_NONE,		and_handler)										\
	X(OR,		1, OP_ARG_NONE,		or_handler)											\
	X(XOR,		1, OP_ARG_NONE,		xor_handler)										\
	X(NOT,		1, OP_ARG_NONE,		not_handler)										\
	X(SHL,		1, OP_ARG_NONE,		shl_handler)										\
	X(SHR,		1, OP_ARG_NONE,		shr_handler)										\
																						\
	/* работа с памятью */																\
	X(LOAD8,	3, OP_ARG_RR, load8_handler)	/* LOAD regInput, regAddr */			\
	X(LOAD16,	3, OP_ARG_RR, load16_handler)	/* LOAD regInput, regAddr */		\
	X(LOAD32,	3, OP_ARG_RR, load32_handler)	/* LOAD regInput, regAddr */		\
	X(STORE8,	3, OP_ARG_RR, store8_handler)	/* STORE regAddr, regInput */		\
	X(STORE16,	3, OP_ARG_RR, store16_handler)	/* STORE regAddr, regInput */		\
	X(STORE32,	3, OP_ARG_RR, store32_handler)	/* STORE regAddr, regInput */		\
																						\
	X(MOVRR,	3, OP_ARG_RR,		movrr_handler)	/* MOVRR reg1, reg2 */					\
	X(MOVRDW,	6, OP_ARG_R_DWORD,	movrn_handler)	/* MOVRN reg1, num const */				\
	X(SYSCALL,	5, OP_ARG_DWORD,	syscall_handler)	/* SYSCALL reg1 */					\

#define OPCODE_HANDLER(name_, size_, arg_type_, func) \
	opcode_work_status func(struct v_unit* u, decoded_opcode* op);
OPCODE_LIST(OPCODE_HANDLER)
#undef OPCODE_HANDLER

typedef enum {
#define OPCODE_ENUM(name, size_, arg_type_, func) OP_##name,
	OPCODE_LIST(OPCODE_ENUM)
#undef OPCODE_ENUM
	OP_COUNT
} opcode_enum;

extern const opcode_info_t opcode_info_table[];

#endif //UNTITLED_OPCODE_H
