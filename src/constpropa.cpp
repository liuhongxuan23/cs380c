#include <cstdio>
#include "icode.h"

int main() {
	Program prog(stdin);
	prog.constant_propagate();
	prog.ccode(stdout);
	return 0;
}
