
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <climits>
#include <set>

#include "icode.h"

bool output_report = false;

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

void Operand::icode (FILE *out) const
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
		if (tag.empty())
			fprintf(out, "%lld", value);
		else if (ssa_idx == -1)
			fprintf(out, "%s#%lld", tag.c_str(), value);
                else
                        fprintf(out, "%s$%d", tag.c_str(), ssa_idx);
	}
}

void Operand::ccode (FILE *out) const
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
		oper[0] = Operand(buf);
	}
	if (op.operands() >= 2) {
		fscanf(in, "%200s", buf);
		oper[1] = Operand(buf);
	}
}

void Instruction::icode (FILE *out) const
{
	fprintf(out, "instr %lld: %s", addr, op.name());
	if (oper[0]) {
		fprintf(out, " ");
		oper[0].icode(out);
	}
	if (oper[1]) {
		fprintf(out, " ");
		oper[1].icode(out);
	}
	fprintf(out, "\n");
}

void Instruction::ccode (FILE *out) const
{
	if (op == Opcode::ENTER) {
		fprintf(out, "void instr_%lld() {\n", addr);
	} else if (op == Opcode::ENTRYPC) {
		fprintf(out, "void instr_%lld();\nvoid (*entry)() = instr_%lld", addr + 1, addr + 1);
	} else {
		fprintf(out, "instr_%lld: ", addr);
	}

	switch(op) {
	case Opcode::ADD:
		fprintf(out, "r[%lld] = ", addr); oper[0].ccode(out); fprintf(out, " + "); oper[1].ccode(out);
		break;
	case Opcode::SUB:
		fprintf(out, "r[%lld] = ", addr); oper[0].ccode(out); fprintf(out, " - "); oper[1].ccode(out);
		break;
	case Opcode::MUL:
		fprintf(out, "r[%lld] = ", addr); oper[0].ccode(out); fprintf(out, " * "); oper[1].ccode(out);
		break;
	case Opcode::DIV:
		fprintf(out, "r[%lld] = ", addr); oper[0].ccode(out); fprintf(out, " / "); oper[1].ccode(out);
		break;
	case Opcode::MOD:
		fprintf(out, "r[%lld] = ", addr); oper[0].ccode(out); fprintf(out, " %% "); oper[1].ccode(out);
		break;
	case Opcode::NEG:
		fprintf(out, "r[%lld] = ", addr); fprintf(out, "- "); oper[0].ccode(out);
		break;
	case Opcode::CMPEQ:
		fprintf(out, "r[%lld] = ", addr); oper[0].ccode(out); fprintf(out, " == "); oper[1].ccode(out);
		break;
	case Opcode::CMPLE:
		fprintf(out, "r[%lld] = ", addr); oper[0].ccode(out); fprintf(out, " <= "); oper[1].ccode(out);
		break;
	case Opcode::CMPLT:
		fprintf(out, "r[%lld] = ", addr); oper[0].ccode(out); fprintf(out, " < "); oper[1].ccode(out);
		break;
	case Opcode::BR:
		fprintf(out, "goto "); oper[0].ccode(out);
		break;
	case Opcode::BLBC:
		fprintf(out, "if ("); oper[0].ccode(out); fprintf(out, "== 0) goto "); oper[1].ccode(out);
		break;
	case Opcode::BLBS:
		fprintf(out, "if ("); oper[0].ccode(out); fprintf(out, "!= 0) goto "); oper[1].ccode(out);
		break;
	case Opcode::CALL:
		fprintf(out, "SP -= 8; MEM(SP) = %lld + 1; ", addr); oper[0].ccode(out); fprintf(out, "()");
		break;
	case Opcode::LOAD:
		fprintf(out, "r[%lld] = MEM(", addr); oper[0].ccode(out); fprintf(out, ")");
		break;
	case Opcode::STORE:
		fprintf(out, "MEM("); oper[1].ccode(out); fprintf(out, ") = "); oper[0].ccode(out);
		break;
	case Opcode::MOVE:
		fprintf(out, "r[%lld] = ", addr); oper[1].ccode(out); fprintf(out, " = "); oper[0].ccode(out);
		break;
	case Opcode::READ:
		fprintf(out, "ReadLong(r[%lld])", addr);
		break;
	case Opcode::WRITE:
		fprintf(out, "WriteLong("); oper[0].ccode(out); fprintf(out, ")");
		break;
	case Opcode::WRL:
		fprintf(out, "WriteLine()");
		break;
	case Opcode::PARAM:
		fprintf(out, "SP -= 8; MEM(SP) = "); oper[0].ccode(out);
		break;
	case Opcode::ENTER:
		fprintf(out, "SP -= 8; MEM(SP) = FP; FP = SP; SP -= "); oper[0].ccode(out);
		break;
	case Opcode::RET:
		fprintf(out, "SP = FP + 16 + "); oper[0].ccode(out); fprintf(out, "; FP = MEM(FP)");
		break;
	}

	fprintf(out, ";\n");
	if (op == Opcode::RET) {
		fprintf(out, "}\n");
	}
}

int Instruction::get_branch_target () const
{
	if (op.type == Opcode::BR)
		return oper[0].value;
	if (op.type == Opcode::BLBC || op.type == Opcode::BLBS)
		return oper[1].value;
	if (op.type == Opcode::RET)
		return 0;
	return -1;
}

int Instruction::get_next_instr() const
{
	if (op == Opcode::BR || op == Opcode::RET)
		return -1;
	else
		return addr + 1;
}

bool Instruction::isconst() const {
	switch(op) {
	case Opcode::ADD:
	case Opcode::SUB:
	case Opcode::MUL:
	case Opcode::DIV:
	case Opcode::MOD:
	case Opcode::CMPEQ:
	case Opcode::CMPLE:
	case Opcode::CMPLT:
		return oper[0].type == Operand::CONST && oper[1].type == Operand::CONST;
	case Opcode::NEG:
	case Opcode::MOVE:
		return oper[0].type == Operand::CONST;
	}
	return false;
}

long long Instruction::constvalue() const {
	if (!isconst()) return 0;
	switch(op) {
	case Opcode::ADD:
		return oper[0].value + oper[1].value;
	case Opcode::SUB:
		return oper[0].value - oper[1].value;
	case Opcode::MUL:
		return oper[0].value * oper[1].value;
	case Opcode::DIV:
		return oper[0].value / oper[1].value;
	case Opcode::MOD:
		return oper[0].value % oper[1].value;
	case Opcode::CMPEQ:
		return oper[0].value == oper[1].value;
	case Opcode::CMPLE:
		return oper[0].value <= oper[1].value;
	case Opcode::CMPLT:
		return oper[0].value < oper[1].value;
	case Opcode::NEG:
		return -oper[0].value;
	case Opcode::MOVE:
		return oper[0].value;
	}
	return 0;
}

bool Instruction::isrightvalue(int o) const
{
	switch (o) {
	case 0:
		switch (op) {
		case Opcode::ADD:
		case Opcode::SUB:
		case Opcode::MUL:
		case Opcode::DIV:
		case Opcode::MOD:
		case Opcode::NEG:
		case Opcode::CMPEQ:
		case Opcode::CMPLE:
		case Opcode::CMPLT:
		case Opcode::BLBC:
		case Opcode::BLBS:
		case Opcode::LOAD:
		case Opcode::STORE:
		case Opcode::MOVE:
		case Opcode::WRITE:
		case Opcode::PARAM:
			return true;
		}
		return false;
	case 1:
		switch (op) {
		case Opcode::ADD:
		case Opcode::SUB:
		case Opcode::MUL:
		case Opcode::DIV:
		case Opcode::MOD:
		case Opcode::CMPEQ:
		case Opcode::CMPLE:
		case Opcode::CMPLT:
		case Opcode::STORE:
			return true;
		}
		return false;
	}
	return false;
}

bool Instruction::eliminable() const
{
	switch (op) {
	case Opcode::ADD:
	case Opcode::SUB:
	case Opcode::MUL:
	case Opcode::DIV:
	case Opcode::MOD:
	case Opcode::NEG:
	case Opcode::CMPEQ:
	case Opcode::CMPLE:
	case Opcode::CMPLT:
	case Opcode::LOAD:
	case Opcode::MOVE:
		return true;
	}
	return false;
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

void Program::icode (FILE *out) const
{
	for (auto p = ++instr.begin(); p != instr.end(); ++p)
		p->icode(out);
}

void Program::ccode (FILE *out) const
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


	bool in_function = false;
	for (auto p = ++instr.begin(); p != instr.end(); ++p) {
		if (p->op == Opcode::ENTER) {
			in_function = true;
		} else if (p->op == Opcode::RET) {
			in_function = false;
		}
		if (in_function || p->op != Opcode::NOP) {
			p->ccode(out);
		}
	}

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

void Program::build_domtree()
{
	for (Function *f: funcs)
		f->build_domtree();
}

void Program::constant_propagate()
{
	for (Function *f: funcs)
		f->constant_propagate();
}

void Program::dead_eliminate()
{
	for (Function *f: funcs)
		f->dead_eliminate();
}

Program::~Program()
{
    for (Function* func : funcs)
        delete func;
}

Function::Function(Program* prog, int a, int b)
    : prog(prog), enter(a), exit(b)
{
    assert(prog->instr[enter].op == Opcode::ENTER);
    frame_size = prog->instr[enter].oper[0].value / 8;
    arg_count = prog->instr[exit].oper[0].value / 8;
    is_main = prog->instr[enter - 1].op == Opcode::ENTRYPC;

    std::set<int> bounds;  // boundaries of blocks

    for (int i = enter; i <= exit; ++i) {
        int br = prog->instr[i].get_branch_target();
        if (br != -1 || prog->instr[i].op == Opcode::CALL)
            bounds.insert(i + 1);
        if (br > 0)
            bounds.insert(br);
    }

    int begin = enter;
    for (int end : bounds) {
        auto it = prog->instr.cbegin();
        blocks[begin] = new Block(this, std::vector<Instruction>(it + begin, it + end));
        begin = end;
    }
    entry = blocks[enter];

    begin = enter;
    for (int end : bounds) {
        Block* b = blocks[begin];

        Opcode::Type op = b->instr.back().op;
        b->seq_next = (op != Opcode::BR && op != Opcode::RET) ? blocks[end] : nullptr;
	if (b->seq_next)
		b->seq_next->prevs.push_back(b);

        int br = b->instr.back().get_branch_target();
        b->br_next = (br > 0) ? blocks[br] : nullptr;
	if (b->br_next && b->br_next != b->seq_next)
		b->br_next->prevs.push_back(b);

        begin = end;
    }
}

Function::~Function()
{
    for (const auto& iter : blocks)
        delete iter.second;
}

void Function::build_domtree()
{
	typedef std::set<Block*> blockset;
	std::map<Block*, blockset> dominators;
	dominators[entry] = blockset();
	/* uninitialized set denotes complete set */
	bool change;
	do {
		change = false;
		for (auto pb: blocks) {
			Block *b = pb.second;
			if (dominators.find(b) == dominators.end()) {
				bool found = false;
				for (Block *p: b->prevs) {
					auto pps = dominators.find(p);
					if (pps != dominators.end()) {
						dominators[b] = pps->second;
						dominators[b].insert(p);
						change = true;
						found = true;
						break;
					}
				}
				if (!found)
					continue;
			}
			blockset &bs = dominators.find(b)->second;
			for (auto p: b->prevs) {
				auto pps = dominators.find(p);
				if (pps != dominators.end()) {
					blockset &ps = pps->second;
					std::list<Block*> erase;
					for (Block *i: bs) {
						if (i != p && ps.find(i) == ps.end()) {
							erase.push_back(i);
							change = true;
						}
					}
					for (Block *i: erase)
						bs.erase(i);
				}
			}
		}
	} while(change);
	for (auto pb: blocks) {
		Block *b = pb.second;
		b->idom = NULL;
		b->domc.clear();
	}
	for (auto pb: blocks) {
		Block *b = pb.second;
		blockset &bs = dominators.find(b)->second;
		for (Block *p: bs) {
			blockset &ps = dominators.find(p)->second;
			for (Block *i: ps)
				bs.erase(i);
		}
		assert(bs.size() <= 1);
		for (Block *p: bs) {
			b->idom = p;
			p->domc.push_back(b);
		}
	}
}

void Function::constant_propagate()
{
	typedef std::set<std::pair<int, int> > iset; // Set of variable definitions
	int propa_count = 0;
	std::vector<Instruction> &instr = prog->instr;
	std::map<Block*, iset> rd; // Reaching Definition IN
	for (auto pb: blocks) {
		Block *b = pb.second;
		rd[b] = iset();
	}
	for (int i = 0; i < arg_count; ++i)
		// Insert function agruments
		// Instruction after enter
		rd[blocks[enter]].insert(std::make_pair((i+2)*8, enter));
	bool change;
	do {
		change = false;
		for (auto pb: blocks) {
			Block *b = pb.second;
			iset cur = rd[b]; // Copy
			for (int j = 0; j < b->instr.size(); ++j) {
				int i = b->instr[j].addr; // Instruction address;
				Instruction &ins = instr[i]; // Instruction in Program
				if (ins.op == Opcode::MOVE && ins.oper[1] == Operand::LOCAL) {
					int def = ins.oper[1].value;
					auto vst = cur.lower_bound(std::make_pair(def, INT_MIN));
					auto ved = cur.lower_bound(std::make_pair(def, INT_MAX));
					cur.erase(vst, ved);
					cur.insert(std::make_pair(def, i));
				}
			}
			for (int t = 0; t < 2; ++t) {
				Block *b2 = t == 0 ? b->seq_next : b->br_next;
				if (b2 != NULL) {
					iset &in2 = rd[b2];
					for (auto i: cur) if (in2.find(i) == in2.end()) {
						change = true;
						in2.insert(i);
					}
				}
			}
		}
	} while (change);
	do {
		change = false;
		for (auto pb: blocks) {
			Block *b = pb.second;
			iset cur = rd[b]; // Copy
			for (int j = 0; j < b->instr.size(); ++j) {
				int i = b->instr[j].addr; // Instruction address;
				Instruction &ins = instr[i]; // Instruction in Program
				for (int o = 0; o < 2; ++o) if (ins.isrightvalue(o)) switch(ins.oper[o].type) {
				case Operand::LOCAL: {
					int var = ins.oper[o].value;
					auto vst = cur.lower_bound(std::make_pair(var, INT_MIN));
					auto ved = cur.lower_bound(std::make_pair(var, INT_MAX));
					bool flag = true;
					long long value = 0;
					for (auto v = vst; v != ved; ++v) {
						Instruction &ins2 = instr[v->second];
						assert(ins2.op == Opcode::MOVE || ins2.op == Opcode::ENTER);
						if (ins2.op == Opcode::MOVE && ins2.oper[0].type == Operand::CONST) {
							if (v == vst) {
								value = ins2.oper[0].value;
							} else if (value != ins2.oper[0].value) {
								flag = false;
							}
						} else {
							flag = false;
						}
						if (!flag)
							break;
					}
					if (flag) {
						ins.oper[o] = Operand();
						ins.oper[o].type = Operand::CONST;
						ins.oper[o].value = value;
						change = true;
						propa_count++;
					}
					break;
				}
				case Operand::REG:{
					Instruction &ins2 = instr[ins.oper[o].value];
					if (ins2.isconst()) {
						ins.oper[o] = Operand();
						ins.oper[o].type = Operand::CONST;
						ins.oper[o].value = ins2.constvalue();
						change = true;
						propa_count++;
					}
				}
				}
				if (ins.op == Opcode::MOVE && ins.oper[1] == Operand::LOCAL) {
					int def = ins.oper[1].value;
					auto vst = cur.lower_bound(std::make_pair(def, INT_MIN));
					auto ved = cur.lower_bound(std::make_pair(def, INT_MAX));
					cur.erase(vst, ved);
					cur.insert(std::make_pair(def, i));
				}
			}
		}
	} while(change);
	if (output_report) {
		printf("Function: %d\n", enter);
		printf("Number of constants propagated: %d\n", propa_count);
	}
}

void Function::dead_eliminate()
{
	typedef std::set<int> iset; // Set of variable definitions
	int elimin_count = 0;
	std::vector<Instruction> &instr = prog->instr;
	std::map<Block*, iset> lv; // Live Variables OUT
	std::set<int> lvreg; // Live Temporary Registers
	for (auto pb: blocks) {
		Block *b = pb.second;
		lv[b] = iset();
	}
	bool change;
	do {
		change = false;
		for (auto ib = blocks.rbegin(); ib != blocks.rend(); ++ib) {
			Block *b = ib->second;
			iset cur = lv[b]; // Copy
			for (int j = (int)b->instr.size() - 1; j >= 0; --j) {
				int i = b->instr[j].addr; // Instruction address;
				Instruction &ins = instr[i]; // Instruction in Program
				bool live = false;
				if (!ins.eliminable() || lvreg.find(i) != lvreg.end()) {
					live = true;
				}
				if (ins.op == Opcode::MOVE && ins.oper[1] == Operand::LOCAL) {
					int def = ins.oper[1].value;
					if (cur.find(def) != cur.end()) {
						cur.erase(def);
						live = true;
					}
				}
				if (live) for (int o = 0; o < 2; ++o) if (ins.isrightvalue(o)) {
					switch(ins.oper[o]) {
					case Operand::LOCAL: {
						int use = ins.oper[o].value;
						cur.insert(use);
						break;
					}
					case Operand::REG: {
						int reg = ins.oper[o].value;
						if (lvreg.find(reg) == lvreg.end()) {
							lvreg.insert(reg);
							change = true;
						}
						break;
					}
					}
				}
			}
			for (Block *b2: b->prevs) {
				iset &out2 = lv[b2];
				for (auto i: cur) if (out2.find(i) == out2.end()) {
					change = true;
					out2.insert(i);
				}
			}
		}
	} while (change);
	for (auto ib = blocks.rbegin(); ib != blocks.rend(); ++ib) {
		Block *b = ib->second;
		iset cur = lv[b]; // Copy
		for (int j = (int)b->instr.size() - 1; j >= 0; --j) {
			int i = b->instr[j].addr; // Instruction address;
			Instruction &ins = instr[i]; // Instruction in Program
			bool live = false;
			if (!ins.eliminable() || lvreg.find(i) != lvreg.end()) {
				live = true;
			}
			if (ins.op == Opcode::MOVE && ins.oper[1] == Operand::LOCAL) {
				int def = ins.oper[1].value;
				if (cur.find(def) != cur.end()) {
					cur.erase(def);
					live = true;
				}
			}
			if (live) {
				for (int o = 0; o < 2; ++o) if (ins.isrightvalue(o)) {
					switch(ins.oper[o]) {
					case Operand::LOCAL: {
						int use = ins.oper[o].value;
						cur.insert(use);
						break;
					}
					}
				}
			} else {
				/* Eliminate */
				long long back_addr = ins.addr;
				ins = Instruction();
				ins.op.type = Opcode::NOP;
				ins.addr = back_addr;
				++elimin_count;
			}
		}
	}
	if (output_report) {
		fprintf(stderr, "Function: %d\n", enter);
		fprintf(stderr, "Number of statements eliminated: %d\n", elimin_count);
	}
}

Block::Block(Function* func, std::vector<Instruction> instr_)
    : func(func)
{
    instr.swap(instr_);
}
