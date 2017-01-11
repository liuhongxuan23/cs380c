#include <cstdio>
#include "icode.h"

int main() {
    Program prog(stdin);
    prog.find_functions();

    for (auto func : prog.funcs) {
        printf("Function: %lld\n", func->blocks.cbegin()->second->instr[0].addr);

        printf("Basic blocks:");
        for (auto iter : func->blocks)
            printf(" %lld", iter.second->instr[0].addr);

        printf("\nCFG:\n");
        for (auto iter : func->blocks) {
            Block* b = iter.second;
            printf("%lld ->", b->instr[0].addr);
            if (b->seq_next != nullptr)
                printf(" %lld", b->seq_next->instr[0].addr);
            if (b->br_next != nullptr)
                printf(" %lld", b->br_next->instr[0].addr);
            putchar('\n');
        }
    }

    return 0;
}
