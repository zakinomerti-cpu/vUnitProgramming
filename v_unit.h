#ifndef VIRTCPU_VUNIT_H
#define VIRTCPU_VUNIT_H
#include <stdint.h>

#define REGS_COUNT 16
#define FREE_MEMORY_MIN 1024
#define WINDOW_BASE_SIZE 32768
#define VU_ARGS_HEADER_MAGIC {0x56, 0x55, 0x5F, 0x41, 0x52, 0x47, 0x48, 0x5F}
#define VU_VAR_KEY_LEN 6
#define VU_VAR_MAX 256
typedef struct v_unit v_unit;

typedef enum {
    VU_MSG_CALL = 0,
    VU_MSG_RETURN,
    VU_MSG_SIGNAL,
    VU_MSG_EXECUTABLE_CODE,
    VU_MSG_STATE_SWITCH,
} vu_msg_type;

typedef enum {
    DIR_OUT,
    DIR_IN,
} vu_msg_direction;

#pragma pack(push,1)
struct exitState {
    int32_t     regs[16];
    uint8_t     memptr[8];
    uint32_t    pc;
    uint32_t    mp;
    uint32_t    fault_addr;
    uint8_t     exit_code;
    int8_t      cmpResult;
    int8_t      fault_mode;
    uint8_t     lastProg[384];
    uint8_t     reserved[41];
};
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
    uint8_t     magicN[8];
    uint8_t     version;
    uint8_t     msg_type; //vu_msg_type
    uint8_t     msg_dir;
    uint16_t    sender_id;
    uint16_t    target_id;
    uint8_t     reserved0[1];
    uint16_t    args_size;
    uint8_t     exit_code;
    uint32_t    fault_address;
    uint8_t     reserved1[9];
    uint8_t     args[256];
    struct      exitState state;
    uint8_t     reserved2[224];
} vu_arg_header;
#pragma pack(pop)

typedef enum vu_exit_code {
    VU_EXIT_NONE = 0,
    VU_EXIT_HALT,
    VU_EXIT_END_OF_PROGRAM,
    VU_EXIT_BREAKPOINT,
    VU_EXIT_STEP_LIMIT,
    VU_EXIT_INVALID_OPCODE,
    VU_EXIT_INVALID_OPERAND,
    VU_EXIT_BAD_REGISTER,
    VU_EXIT_MEMORY_READ,
    VU_EXIT_MEMORY_WRITE,
    VU_EXIT_MEMORY_EXEC,
    VU_EXIT_DIVIDE_BY_ZERO,
    VU_EXIT_ILLEGAL_JUMP,
    VU_EXIT_INTERNAL_ERROR,
} vu_exit_code;

typedef enum {
    VU_OK = 0,
    VU_ERR_OOB,
    VU_ERR_PERMISSION_DENIED,
    VU_ERR_BAD_INDEX,
    VU_ERR_WND_EXEC_CODE_BIGGER_THAN_PROG_RAM
} vu_status;

typedef struct {
    uint8_t flag[8];
    uint32_t size;
    uint32_t offset;
} vu_mem;

typedef struct {
    char key[VU_VAR_KEY_LEN*VU_VAR_MAX];
    int32_t key_count;
    int32_t value[VU_VAR_MAX];
} vu_table;

struct vu_ops {
    int (*step)(v_unit* u);
    int (*execute)(
        v_unit* u,
        uint32_t max_steps
    );
    void (*reset)(
        v_unit* u
    );

    vu_status (*executeFromWindow)(
        v_unit* u
    );

    vu_status (*flashDevice)(
        v_unit* u
    );

    // работа с окном
    vu_status(*window_data_push)(
        v_unit* u,
        uint8_t b
    );
    vu_status(*window_data_pop)(
        v_unit* u,
        uint8_t* out
    );
    vu_status(*window_args_push)(
        v_unit* u,
        uint8_t b
    );
    vu_status(*window_args_pop)(
        v_unit* u,
        uint8_t* out
    );
    vu_status(*window_data_write)(
        v_unit* u,
        uint32_t innerIndex,
        const uint8_t* src,
        uint32_t len
    );
    vu_status(*window_data_read)(
        v_unit* u,
        uint32_t innerIndex,
        uint8_t* dst,
        uint32_t len
    );
    vu_status(*window_args_write)(
        v_unit* u,
        uint32_t innerIndex,
        const uint8_t* src,
        uint32_t len
    );
    vu_status(*window_args_read)(
        v_unit* u,
        uint32_t innerIndex,
        uint8_t* dst,
        uint32_t len
    );
    vu_status(*window_dump)(
        v_unit* u,
        char* dst,
        int32_t dst_cap,
        int32_t* written
    );
    vu_status(*reg_set)(
        v_unit* u,
        uint8_t reg,
        uint32_t value
    );
    vu_status(*reg_get)(
        v_unit* u,
        uint8_t reg,
        uint32_t* out
    );
    vu_status(*reg_dump)(
        v_unit* u,
        char* dst,
        int32_t dst_cap,
        int32_t* written
    );
    vu_status(*var_set)(
        v_unit* u,
        char (*key)[6],
        uint32_t value
    );
    vu_status(*var_get)(
        v_unit* u,
        char (*key)[6],
        uint32_t* value
    );
    vu_status(*var_dump)(
        v_unit* u,
        char* dst,
        int32_t dst_cap,
        int32_t* written
    );
    void (*unit_dump)(
        v_unit* u,
        char* dst,
        int32_t dst_cap,
        int32_t* written
    );
};

struct v_unit {
    struct vu_ops* ops;
    vu_exit_code exitCode;
    uint32_t pc;
    uint32_t dp;
    uint32_t wp;
    uint32_t fault_addr;
    int8_t cmpResult; /* -1, 0, 1 */

    /* 0 - упасть с ошибкой */
    /* 1 продолжить с ошибкой(пропуск инструкции) */
    uint8_t fault_mode;

    vu_mem regs;
    vu_mem program;
    vu_mem window_args;
    vu_mem window_data;
    vu_mem data;

    vu_table opcodes;
    vu_table vars;
};

typedef enum {
    OP_HALT = 0,
    OP_NOP,
    OP_JMP, /* jmp addr(4) */
    OP_JZ,
    OP_JP,
    OP_JN,
    OP_CMPVV,
    OP_CMPRR,
    OP_CMPRV,
    OP_CMPVR,

    /* арифметика */
    OP_ADD_R,
    OP_SUB_R,
    OP_MUL_R,
    OP_DIV_R,
    OP_MOD_R,
    OP_NEG_R,

    OP_ADD_V,
    OP_SUB_V,
    OP_MUL_V,
    OP_DIV_V,
    OP_MOD_V,
    OP_NEG_V,

    OP_ADD_VR,
    OP_SUB_VR,
    OP_MUL_VR,
    OP_DIV_VR,
    OP_MOD_VR,

    OP_ADD_RV,
    OP_SUB_RV,
    OP_MUL_RV,
    OP_DIV_RV,
    OP_MOD_RV,


    /* битовые операции, только регистры */
    OP_AND,
    OP_OR,
    OP_XOR,
    OP_NOT,
    OP_SHL,
    OP_SHR,

    OP_LOAD8,
    OP_LOAD16,
    OP_LOAD32,
    OP_STORE8,
    OP_STORE16,
    OP_STORE32,

    OP_VTOR,
    OP_RTOV,
    OP_SETR,
    OP_SETV, //setv var(6) value(4)
    OP_NEWVAR, //newvar var(6) value(4)

    OP_LDWND, // ldwnd addr
    OP_STWND, // stwnd addr

    OP_DBGVAR,
    OP_DBGREG,
    OP_DBGWND,
    OP_DBGPRG,
    OP_DBGDAT,
    OP_DBGSYS
} vu_opcodes;

v_unit* v_unit_new(
    int data_size,
    int prog_size
);

#endif //VIRTCPU_VUNIT_H
