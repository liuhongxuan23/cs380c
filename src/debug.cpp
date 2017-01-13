#include "icode.h"

#include <cstdio>

void print_cfg(Program *prog) {
	for (Function *func: prog->funcs) {
		printf("Function: %d\n", func->name);

		printf("Basic blocks:");
		for (Block *b = func->entry; b != NULL; b = b->order_next)
			printf(" %d", b->name);

		printf("\nCFG:\n");
		for (Block *b = func->entry; b != NULL; b = b->order_next) {
			printf("%d ->", b->name);
			if (b->seq_next != nullptr)
				printf(" %d", b->seq_next->name);
			if (b->br_next != nullptr)
				printf(" %d", b->br_next->name);
			putchar('\n');
		}
	}
}

int main() {
    Program prog(fopen("debug.txt", "r"));

    prog.build_domtree();

    prog.ssa_prepare();
    //prog.find_defs();
    //prog.place_phi();
    //prog.ssa_rename_var();

    //prog.ssa_licm();
    //print_cfg(&prog);

    //prog.remove_phi();
    //prog.ssa_icode(stdout);

    prog.ssa_constant_propagate();
    prog.ssa_licm();
    prog.ssa_to_3addr();
    prog.rename();
    prog.icode(stdout);

    //prog.icode(stdout);

    //    printf("Basic blocks:");
    //    for (auto iter : func->blocks)
    //        printf(" %lld", iter.second->addr());

    //    printf("\nCFG:\n");
    //    for (auto iter : func->blocks) {
    //        Block* b = iter.second;
    //        printf("%lld ->", b->addr());
    //        if (b->seq_next != nullptr)
    //            printf(" %lld", b->seq_next->addr());
    //        if (b->br_next != nullptr)
    //            printf(" %lld", b->br_next->addr());
    //        putchar('\n');
    //    }

    //    printf("\nDom Tree:\n");
    //    for (auto iter : func->blocks) {
    //        Block* b = iter.second;
    //        printf("%lld ->", b->addr());
    //        for (Block *c: b->domc)
    //            printf(" %lld", c->addr());
    //        putchar('\n');
    //    }

    //for (Function* f : prog.funcs) {
    //    printf("\nDF:\n");
    //    for (Block* b : f->blocks) {
    //        printf("%lld :", b->name);
    //        for (Block *df: b->df)
    //            printf(" %lld", df->name);
    //        putchar('\n');
    //    }

    //    printf("\nPhi:\n");
    //    for (Block* b : f->blocks) {
    //        printf("%lld :\n", b->name);
    //        for (const auto& var_phi : b->phi) {
    //            Localvar* var = var_phi.first;
    //            const Phi& phi = var_phi.second;

    //            printf("  [%s] %d <-", var->name.c_str(), phi.l);
    //            for (auto op : phi.r)
    //                printf(" %d", op.ssa_idx);
    //            putchar('\n');
    //        }
    //        putchar('\n');
    //    }
    //}

    return 0;
}
