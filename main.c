#include <stdio.h>

#include "opcodes/opcode.h"
#include "v_unit.h"
#include "stdlib/vu_string.h"
#include "core/pVoidArray/pVoidArray.h"
int main() {

    v_unit* v = NULL;
    v_unit_new(128, &v);

    uint8_t program[] = {
        OP_MOVRDW, 0, 1, 0, 0, 0,    //6
        OP_MOVRDW, 1, 11, 0, 0, 0,   //6
        OP_MOVRDW, 2, 3, 0, 0, 0,    //6
        OP_ADD, 0, 2,               //3
        OP_INC, 0,                  //2
        OP_CMPRR, 0, 1,             //3
        OP_JN, 18, 0, 0, 0,         //5
        OP_HALT,
    };
    vu_memcpy(v->memory.addr, program, sizeof(program));

    char buffer[2048];
    v->ops->debug_memory(v, sizeof(buffer), buffer, NULL);
    printf("%s", buffer);

    for (int i = 0; i < 1024; i++) {
        v->ops->step(v);
    }

    v->ops->debug_registers(v, sizeof(buffer), buffer, NULL);
    printf("%s", buffer);

    return 0;
}