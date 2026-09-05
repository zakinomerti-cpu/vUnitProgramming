#ifndef VIRTCPU_VUNIT_H
#define VIRTCPU_VUNIT_H
#include <stdint.h>

typedef struct vUnitBase vUnitBase;

typedef struct {

    //execution block
    int (*execute)(vUnitBase* vCpu);

    //registers
    int32_t (*getRegValue)(vUnitBase* vCpu, unsigned char reg);

    //*memory
    void (*pushByte)(vUnitBase*, unsigned char byte);
    void (*pushData)(vUnitBase*, void* data, int size);
    void (*setByte)(vUnitBase*, unsigned char byte, int index);
    void (*setData)(vUnitBase*, void* data, int size, int index);
    void (*memory_dump)(vUnitBase*);
} vUnitBaseInterface;

struct vUnitBase {
    const vUnitBaseInterface* ops;
    uint32_t halted;
    uint32_t pc; /* program counter */
    uint32_t sp; /* stack pointer */
    uint32_t mp; /* memory pointer( last unused byte ) */
    int32_t outReg[4];

    uint32_t regs_count;
    uint32_t stack_size;
    uint32_t memory_size;

    uint32_t regs_offset;
    uint32_t stack_offset;
    uint32_t memory_offset;
    // далее идут regs, stack, memory.
};

typedef enum {
    OP_HALT = 0,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_NEG,
    OP_NOT,
    OP_JMP,
    OP_CMP, /* CMP rresult, r1, r2 */
    OP_PUSH,
    OP_POP,
    OP_LOAD_8,
    OP_LOAD_16,
    OP_LOAD_32,
    OP_STORE_8,
    OP_STORE_16,
    OP_STORE_32
} vUnitBaseOpcodes;

vUnitBase* vUnitBase_new(
    int numOfRegs,
    int sizeOfStack,
    int sizeOfMemory
);

#endif //VIRTCPU_VUNIT_H
