#ifndef VIRTCPU_VUNIT_H
#define VIRTCPU_VUNIT_H
#include <stdint.h>

#define REGS_COUNT 16
typedef struct v_unit v_unit;

typedef enum vu_exit_code {
    VU_EXIT_NONE,
    VU_EXIT_END_OF_PROGRAM,
    VU_EXIT_BREAKPOINT,
    VU_EXIT_STEP_LIMIT,
    VU_EXIT_OPCODE_ERROR,
} vu_exit_code;

typedef enum {
    VU_CREATE_OK = 0,
    VU_NULL_PARAM,
    VU_CREATE_ALLOCATION_FAILED,
} vu_create_status;

struct vu_ops {
    vu_exit_code (*step)(v_unit* u);
    vu_exit_code (*execute)(
        v_unit* u,
        uint32_t max_steps
    );

    void (*reset)(
        v_unit* u
    );
    void (*debug_registers)(
        v_unit* u,
        uint32_t bufferSize,
        char* buffer,
        uint32_t* written
    );
    void (*debug_memory)(
        v_unit* u,
        uint32_t bufferSize,
        char* buffer,
        uint32_t* written
    );
    void (*debug_program)(
        v_unit* u,
        uint32_t bufferSize,
        char* buffer,
        uint32_t* written
    );
};

typedef struct {
    uint8_t flags;
    uint32_t size;
    uint32_t offset;
    uint8_t* addr;
} vu_mem;

struct v_unit_frame {
    uint32_t ip;
    uint32_t dp;
    uint32_t sp;

    vu_exit_code exit_code;
    int cause_id;
    int8_t cmpResult;

    vu_mem regs;
    vu_mem* stack;
    vu_mem* memory;
};

struct v_unit {
    struct vu_ops* ops;
    uint32_t ip;
    uint32_t sp;
    uint32_t dp;
    uint32_t requestedMemorySize;

    vu_exit_code exit_code;
    int cause_id;

    int8_t cmpResult; /* -1, 0, 1 */

    vu_mem stack;
    struct v_unit_frame frames[256];

    vu_mem regs;
    vu_mem memory;
};

vu_create_status v_unit_new(
    uint32_t memory_size,
    v_unit** unit
);

#endif //VIRTCPU_VUNIT_H
