#include <cstdio>
#include <cstring>
#include <vector>

#include "icode.h"

enum Opt {
	SCP, //simple constant propagation
	DSE, //dead statement elimination
        LICM, // loop invariant code motion
        SSA,
	MAX_OPT,
};

const char *optname[] = {
	[SCP] = "scp",
	[DSE] = "dse",
        [LICM] = "licm",
        [SSA] = "ssa",
};

enum Backend {
	THREEADDR,
	C,
	CFG,
	REP,
	DOM,
        SSA_3ADDR,
	MAX_BACKEND,
};

const char *backendname[] = {
	[THREEADDR] = "3addr",
	[C] = "c",
	[CFG] = "cfg",
	[REP] = "rep",
	[DOM] = "dom",
        [SSA_3ADDR] = "ssa,3addr",
};

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

void print_dom(Program *prog) {
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

		printf("\nDom Tree:\n");
		for (Block *b = func->entry; b != NULL; b = b->order_next) {
			printf("%d ->", b->name);
			for (Block *c: b->domc)
				printf(" %d", c->name);
			putchar('\n');
		}

		printf("\nLoops:\n");
		for (Block *b = func->entry; b != NULL; b = b->order_next) if (func->loops.count(b) > 0) {
			auto s = func->loops[b];
			for (Block *c: s) {
				printf(" %d", c->name);
			}
			putchar('\n');
		}
	}
}
int main(int argc, char **argv) {
	std::vector<Opt> opts;
	Backend b;
	char *opt = NULL;
	char *backend = NULL;
	for (int i = 1; i < argc; ++i) {
		char *equal = strchr(argv[i], '=');
		if (equal == NULL) {
			fprintf(stderr, "Unknown agruments: %s\n", argv[i]);
			return 1;
		}
		int length = equal - argv[i];
		if (length == 4 && strncmp(argv[i], "-opt", 4) == 0) {
			if (opt != NULL) {
				fprintf(stderr, "multiple opt\n");
				return 1;
			}
			opt = equal + 1;
		} else if (length == 8 && strncmp(argv[i], "-backend", 8) == 0) {
			if (backend != NULL) {
				fprintf(stderr, "multiple backend\n");
				return 1;
			}
			backend = equal + 1;
		}
	}
	if (opt) {
		char *s = opt;
		char *dot;
		do {
			dot = strchr(opt, ',');
			if (dot) *dot = '\0';
			int i;
			for (i = 0; i < MAX_OPT; ++i) {
				if (strcmp(s, optname[i]) == 0) {
					opts.push_back((Opt)i);
					break;
				}
			}
			if (i == MAX_OPT) {
				fprintf(stderr, "unknown optimization %s\n", s);
				return 1;
			}
			s = dot + 1;
		} while (dot != NULL);
	}
	if (backend) {
		int i;
		for (i = 0; i < MAX_BACKEND; ++i) {
			if (strcmp(backend, backendname[i]) == 0) {
				b = (Backend)i;
				break;
			}
		}
		if (i == MAX_BACKEND) {
			fprintf(stderr, "unknown backend: %s\n", backend);
			return 1;
		}
	} else {
		fprintf(stderr, "no backend\n");
		return 1;
	}

	if (b == REP) output_report = true;

	Program prog(stdin);
	prog.build_domtree();

        bool ssa_on = false;

	for (Opt o: opts) switch(o) {
	case SCP:
                if (ssa_on) {
                    prog.ssa_constant_propagate();
                 } else
                    prog.constant_propagate();
		break;
	case DSE:
                if (!ssa_on)
                    prog.dead_eliminate();
		break;
        case SSA:
                prog.ssa_prepare();
                ssa_on = true;
                break;
        case LICM:
                if (ssa_on)
                    prog.ssa_licm();
                break;
	}

        if (ssa_on && b != SSA_3ADDR)
            prog.ssa_to_3addr();

	prog.rename();
	switch(b) {
	case THREEADDR:
		prog.icode(stdout);
		break;
	case C:
		prog.ccode(stdout);
		break;
	case CFG:
		print_cfg(&prog);
		break;
	case DOM:
		print_dom(&prog);
		break;
        case SSA_3ADDR:
                prog.icode(stdout);
                break;
	}

	return 0;
}
