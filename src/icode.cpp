
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <set>

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
	[ADD]=2, [SUB]=2, [MUL]=2, [DIV]=2, [MOD]=2, [NEG]=1,
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

void Operand::ccode (FILE *out)
{
	if (type == REG) {
		fprintf(out, "r[%lld]", value);
	} else if (type == LABEL) {
		fprintf(out, "instr_%lld", value);
	} else if (type == GP) {
		fprintf(out, "GP");
	} else if (type == FP) {
		fprintf(out, "FP");
	} else if (type == LOCAL) {
		fprintf(out, "LOCAL(%lld)", value);
	} else if (type == CONST) {
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

void Instruction::ccode (FILE *out)
{
	if (op == Opcode::ENTER) {
		fprintf(out, "void instr_%lld() {\n", addr);
	} else if (op == Opcode::ENTRYPC) {
		fprintf(out, "void instr_%lld();\nvoid (*entry)() = instr_%lld", addr + 1, addr + 1);
	} else if (op == Opcode::NOP) {
		/* Nothing */
	} else {
		fprintf(out, "instr_%lld: ", addr);
	}

	switch(op) {
	case Opcode::ADD:
		fprintf(out, "r[%lld] = ", addr); oper1.ccode(out); fprintf(out, " + "); oper2.ccode(out);
		break;
	case Opcode::SUB:
		fprintf(out, "r[%lld] = ", addr); oper1.ccode(out); fprintf(out, " - "); oper2.ccode(out);
		break;
	case Opcode::MUL:
		fprintf(out, "r[%lld] = ", addr); oper1.ccode(out); fprintf(out, " * "); oper2.ccode(out);
		break;
	case Opcode::DIV:
		fprintf(out, "r[%lld] = ", addr); oper1.ccode(out); fprintf(out, " / "); oper2.ccode(out);
		break;
	case Opcode::MOD:
		fprintf(out, "r[%lld] = ", addr); oper1.ccode(out); fprintf(out, " %% "); oper2.ccode(out);
		break;
	case Opcode::NEG:
		fprintf(out, "r[%lld] = ", addr); fprintf(out, "- "); oper1.ccode(out);
		break;
	case Opcode::CMPEQ:
		fprintf(out, "r[%lld] = ", addr); oper1.ccode(out); fprintf(out, " == "); oper2.ccode(out);
		break;
	case Opcode::CMPLE:
		fprintf(out, "r[%lld] = ", addr); oper1.ccode(out); fprintf(out, " <= "); oper2.ccode(out);
		break;
	case Opcode::CMPLT:
		fprintf(out, "r[%lld] = ", addr); oper1.ccode(out); fprintf(out, " < "); oper2.ccode(out);
		break;
	case Opcode::BR:
		fprintf(out, "goto "); oper1.ccode(out);
		break;
	case Opcode::BLBC:
		fprintf(out, "if ("); oper1.ccode(out); fprintf(out, "== 0) goto "); oper2.ccode(out);
		break;
	case Opcode::BLBS:
		fprintf(out, "if ("); oper1.ccode(out); fprintf(out, "!= 0) goto "); oper2.ccode(out);
		break;
	case Opcode::CALL:
		fprintf(out, "SP -= 8; MEM(SP) = %lld + 1; ", addr); oper1.ccode(out); fprintf(out, "()");
		break;
	case Opcode::LOAD:
		fprintf(out, "r[%lld] = MEM(", addr); oper1.ccode(out); fprintf(out, ")");
		break;
	case Opcode::STORE:
		fprintf(out, "MEM("); oper2.ccode(out); fprintf(out, ") = "); oper1.ccode(out);
		break;
	case Opcode::MOVE:
		fprintf(out, "r[%lld] = ", addr); oper2.ccode(out); fprintf(out, " = "); oper1.ccode(out);
		break;
	case Opcode::READ:
		fprintf(out, "ReadLong(r[%lld])", addr);
		break;
	case Opcode::WRITE:
		fprintf(out, "WriteLong("); oper1.ccode(out); fprintf(out, ")");
		break;
	case Opcode::WRL:
		fprintf(out, "WriteLine()");
		break;
	case Opcode::PARAM:
		fprintf(out, "SP -= 8; MEM(SP) = "); oper1.ccode(out);
		break;
	case Opcode::ENTER:
		fprintf(out, "SP -= 8; MEM(SP) = FP; FP = SP; SP -= "); oper1.ccode(out);
		break;
	case Opcode::RET:
		fprintf(out, "SP = FP + 16 + "); oper1.ccode(out); fprintf(out, "; FP = MEM(FP)");
		break;
	}

	if (op != Opcode::NOP) {
		fprintf(out, ";\n");
	}
	if (op == Opcode::RET) {
		fprintf(out, "}\n");
	}
}

int Instruction::get_branch_target () const
{
	if (op.type == Opcode::BR)
		return oper1.value;
	if (op.type == Opcode::BLBC || op.type == Opcode::BLBS)
		return oper2.value;
	if (op.type == Opcode::RET)
		return 0;
	return -1;
}

Program::Program (FILE *in):
	Program()
{
	while (!feof(in)) {
		instr.push_back(Instruction(in));
	}
	if (!instr.back())
		instr.pop_back();

        find_functions();
}

void Program::icode (FILE *out)
{
	for (auto p = instr.begin(); p != instr.end(); ++p)
		p->icode(out);
}

void Program::ccode (FILE *out)
{

	fprintf(out, "%s",
	"#include <stdio.h>\n"
	"#define WriteLine() printf(\"\\n\");\n"
	"#define WriteLong(x) printf(\" %lld\", (long)x);\n"
	"#define ReadLong(a) if (fscanf(stdin, \"%lld\", &a) != 1) a = 0;\n"
	"#define long long long\n"
	"#define MEM(a) *(long *)(memory + a)\n"
	"#define LOCAL(a) *(long *)(memory + FP + a)\n"
	"\n"
	"char memory[65536];\n"
	"long r[65536];\n"
	"long GP = 0;\n"
	"long SP = 65536;\n"
	"long FP = 65536;\n"
	"\n"
	);


	for (auto p = instr.begin(); p != instr.end(); ++p)
		p->ccode(out);

	fprintf(out,"void main()\n{\n\t(*entry)();\n}\n");
}

void Program::find_functions()
{
    int enter = -1;

    for (int i = 1; i < instr.size(); ++i) {
        if (instr[i].op == Opcode::ENTER) {
            assert(enter == -1);
            enter = i;

        } else if (instr[i].op == Opcode::RET) {
            assert(enter > 0);
            funcs.push_back(new Function(this, enter, i));
            enter = -1;
        }
    }
}

Program::~Program()
{
    for (Function* func : funcs)
        delete func;
}

Function::Function(Program* prog, int enter, int exit)
    : prog(prog)
{
    assert(prog->instr[enter].op == Opcode::ENTER);
    frame_size = prog->instr[enter].oper1.value;
    is_main = prog->instr[enter - 1].op == Opcode::ENTRYPC;

    std::set<int> bounds;  // boundaries of blocks

    for (int i = enter + 1; i <= exit; ++i) {
        int br = prog->instr[i].get_branch_target();
        if (br != -1)
            bounds.insert(i + 1);
        if (br > 0)
            bounds.insert(br);
    }

    int begin = enter + 1;
    for (int end : bounds) {
        auto it = prog->instr.cbegin();
        blocks[begin] = new Block(this, std::vector<Instruction>(it + begin, it + end));
        begin = end;
    }

    begin = enter + 1;
    for (int end : bounds) {
        Block* b = blocks[begin];

        Opcode::Type op = b->instr.back().op;
        b->seq_next = (op != Opcode::BR && op != Opcode::RET) ? blocks[end] : nullptr;

        int br = b->instr.back().get_branch_target();
        b->br_next = (br > 0) ? blocks[br] : nullptr;

        begin = end;
    }
}

Function::~Function()
{
    for (const auto& iter : blocks)
        delete iter.second;
}

Block::Block(Function* func, std::vector<Instruction> instr_)
    : func(func)
{
    instr.swap(instr_);
}