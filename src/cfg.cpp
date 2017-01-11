#include <cstdio>
#include "icode.h"

int main() {
    Program prog(stdin);

    for (auto func : prog.funcs) {
        printf("Function: %d\n", func->blocks.cbegin()->second->instr[0].addr);

        printf("Basic blocks:");
        for (auto iter : func->blocks)
            printf(" %d", iter.second->instr[0].addr);

        printf("\nCFG:\n");
        for (auto iter : func->blocks) {
            Block* b = iter.second;
            printf("%d ->", b->instr[0].addr);
            if (b->seq_next != nullptr)
                printf(" %d", b->seq_next->instr[0].addr);
            if (b->br_next != nullptr)
                printf(" %d", b->br_next->instr[0].addr);
            putchar('\n');
        }
    }

    return 0;
}
