#include <stdio.h>
#include <string.h>

#include "v_unit.h"

int main() {
    v_unit* v = v_unit_new(34, 1024);

    int c;
    int iter = 0;
    unsigned char prog[1024];
    FILE* f = fopen("output.bin", "r");
    if (!f) return 0;
    while (( c= fgetc(f)) != EOF) {
        prog[iter] = (char)c;
        //printf("%5d", c);
        iter+=1;
        if (iter % 16 == 0 && iter != 0) {
            //printf("\n");
        }
        else {
            //printf(" ");
        }
    }
    //printf("\n");
    fclose(f);

    uint8_t* ptr = (uint8_t*)v;
    memcpy(ptr+v->program.offset, prog, iter);
    for (int i = 0; i < 1024; i++) {
        printf("%d ", *(unsigned char*)(ptr+v->program.offset+i));
    }

    iter = 0;
    printf("\n");
    while (iter < 1024) {
        if (v->ops->step(v) <= 0) {
            printf("%x\n", v->pc);

            break;
        }
        iter+=1;
    }
    return 0;
}
