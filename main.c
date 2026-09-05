#include <stdio.h>

#include "vUnitBase.h"

int main() {
    vUnitBase* unit = vUnitBase_new(
        8,
        1024,
        4096
    );
    int num = 0xFFFA;
    int address = 0x100;

    unit->ops->pushByte(unit, OP_LOAD_32);
    unit->ops->pushByte(unit, 5);
    unit->ops->pushData(unit, &address, 4);
    unit->ops->setData(unit, &num, 4, address);

    while (!unit->halted) {
        unit->ops->execute(unit);
    }

    unit->ops->memory_dump(unit);

    for (int i = 0; i < 8; i++) {
        printf("%x\n", unit->ops->getRegValue(unit, i));
    }
    return 0;
}
