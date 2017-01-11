
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "icode.h"

const char *const Opcode::opname[OPCODE_MAX] = {
	[UNKNOWN]="unknown",
	[ADD]="add",
	[SUB]="sub",
	[MUL]="mul",
	[DIV]="div",
	[MOD]="mod",
	[NEG]="neg",
	[CMPEQ]="cmpeq",
	[CMPLE]="cmple",
	[CMPLT]="cmplt",
	[NOP]="nop",
	[BR]="br",
	[BLBC]="blbc",
	[BLBS]="blbs",
	[CALL]="call",
	[LOAD]="load",
	[STORE]="store",
	[MOVE]="move",
	[READ]="read",
	[WRITE]="write",
	[WRL]="wrl",
	[PARAM]="param",
	[ENTER]="enter",
	[RET]="ret",
	[ENTRYPC]="entrypc",
};

const int Opcode::operand_count[OPCODE_MAX] = {
	[UNKNOWN]=-1,
	[ADD]=2, [SUB]=2, [MUL]=2, [DIV]=2, [MOD]=2, [NEG]=2,
	[CMPEQ]=2, [CMPLE]=2, [CMPLT]=2,
	[NOP]=0,
	[BR]=1, [BLBC]=2, [BLBS]=2,
	[CALL]=1,
	[LOAD]=1,
	[STORE]=2,
	[MOVE]=2,
	[READ]=0,
	[WRITE]=1,
	[WRL]=0,
	[PARAM]=1,
	[ENTER]=1,
	[RET]=1,
	[ENTRYPC]=0,
};

Opcode::Opcode (const char *name):
	Opcode()
{
	for (int i = 1; i < OPCODE_MAX; ++i) {
		if (strcmp(name, opname[i]) == 0) {
			type = (Type) i;
			break;
		}
	}
	if (type == UNKNOWN)
		fprintf(stderr, "Unkown Opcode: %s\n", name);
}

Operand::Operand (const char *str):
	Operand()
{
	if (str == NULL) {
		/* Nothing */
	} else if (str[0] == '(') {
		type = REG;
		value = atoll(str+1);
	} else if (str[0] == '[') {
		type = LABEL;
		value = atoll(str+1);
	} else if (strcmp(str, "GP") == 0) {
		type = GP;
	} else if (strcmp(str, "FP") == 0) {
		type = FP;
	} else {
		const char *sharp = strchr(str, '#');
		if (sharp == NULL) {
			value = atoll(str);
			type = CONST;
		} else {
			value = atoll(sharp+1);
			int len = sharp - str;
			tag.assign(str, len);
			if (len >= 5 && strncmp(str + len - 5, "_base", 5) == 0) {
				type = CONST;
			} else if (len >= 7 && strncmp(str + len - 7 ,"_offset", 7) == 0) {
				type = CONST;
			} else {
				type = LOCAL;
			}
		}
	}
}

void Operand::icode (FILE *out)
{
	if (type == REG) {
		fprintf(out, "(%lld)", value);
	} else if (type == LABEL) {
		fprintf(out, "[%lld]", value);
	} else if (type == GP) {
		fprintf(out, "GP");
	} else if (type == FP) {
		fprintf(out, "FP");
	} else {
		if (!tag.empty())
			fprintf(out, "%s#%lld", tag.c_str(), value);
		else
			fprintf(out, "%lld", value);
	}
}

Instruction::Instruction (FILE *in):
	Instruction()
{
	char buf[201];
	int ret;

	ret = fscanf(in, " instr %lld:", &addr);
	if (ret == EOF)
		return;
	fscanf(in, "%200s", buf);
	op = Opcode(buf);
	if (op.operands() >= 1) {
		fscanf(in, "%200s", buf);
		oper1 = Operand(buf);
	}
	if (op.operands() >= 2) {
		fscanf(in, "%200s", buf);
		oper2 = Operand(buf);
	}
}

void Instruction::icode (FILE *out)
{
	fprintf(out, "instr %lld: %s", addr, op.name());
	if (oper1) {
		fprintf(out, " ");
		oper1.icode(out);
	}
	if (oper2) {
		fprintf(out, " ");
		oper2.icode(out);
	}
	fprintf(out, "\n");
}

Program::Program (FILE *in):
	Program()
{
	while (!feof(in)) {
		instr.push_back(Instruction(in));
	}
	if (!instr.back())
		instr.pop_back();
}

void Program::icode (FILE *out)
{
	for (auto p = instr.begin(); p != instr.end(); ++p)
		p->icode(out);
}
