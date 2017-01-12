#include <cstdio>
#include <cstring>
#include <vector>

#include "icode.h"

enum Opt {
	SCP, //simple constant propagation
	DSL, //dead statement elimination
	MAX_OPT,
};

const char *optname[] = {
	[SCP] = "scp",
	[DSL] = "dsl",
};

enum Backend {
	THREEADDR,
	C,
	CFG,
	REP,
	MAX_BACKEND,
};

const char *backendname[] = {
	[THREEADDR] = "3addr",
	[C] = "c",
	[CFG] = "cfg",
	[REP] = "rep",
};

void print_cfg(Program *prog) {
	for (auto func : prog->funcs) {
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
	prog.find_functions();

	for (Opt o: opts) switch(o) {
	case SCP:
		prog.constant_propagate();
		break;
	case DSL:
		prog.dead_eliminate();
		break;
	}
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
	}

	return 0;
}
