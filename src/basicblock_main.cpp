#include <cstdio>
#include "basicblock.h"

int main() {
    Program prog(stdin);

    Function::init(&prog);
    for (auto func : Function::funcs)
        printf("%d %d\n", func->begin, func->end);
    putchar('\n');
    for (auto it : BasicBlock::blocks)
        printf("%d %d\n", it.second->begin, it.second->end);

    return 0;
}
