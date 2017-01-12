#include <cstdio>
#include "icode.h"

int main() {
	Program prog(stdin);
	prog.find_functions();
	prog.constant_propagate();
	prog.icode(stdout);
	return 0;
}
