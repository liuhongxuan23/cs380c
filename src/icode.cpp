
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <climits>
#include <cstdint>
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

bool Program::ssa_mode;

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
		_value = atoll(str+1);
	} else if (str[0] == '[') {
		type = LABEL;
		_value = atoll(str+1);
	} else if (strcmp(str, "GP") == 0) {
		type = GP;
	} else if (strcmp(str, "FP") == 0) {
		type = FP;
	} else {
		const char *sharp = strchr(str, '#');
		if (sharp == NULL) {
			_value = atoll(str);
			type = CONST;
		} else {
			_value = atoll(sharp+1);
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
	switch (type) {
	case GP:
		fprintf(out, "GP");
		break;
	case FP:
		fprintf(out, "FP");
		break;
	case CONST:
		if (tag.empty())
			fprintf(out, "%lld", value_const);
		else
			fprintf(out, "%s#%lld", tag.c_str(), value_const);
		break;
	case LOCAL:
		if (ssa_idx == -1 || !Program::ssa_mode)
			fprintf(out, "%s#%lld", var->name.c_str(), var->offset);
		else
			fprintf(out, "%s$%d", var->name.c_str(), ssa_idx);
		break;
	case REG:
		fprintf(out, "(%d)", reg->name);
		break;
	case LABEL:
		fprintf(out, "[%d]", jump->name);
		break;
	case FUNC:
		fprintf(out, "[%lld]", value_const);
		break;
	default:
		assert(true);
	}
}

void Operand::ccode (FILE *out) const
{
	switch (type) {
	case GP:
		fprintf(out, "GP");
		break;
	case FP:
		fprintf(out, "FP");
		break;
	case CONST:
		fprintf(out, "%lld", value_const);
		break;
	case LOCAL:
		fprintf(out, "LOCAL(%lld)", var->offset);
		break;
	case REG:
		fprintf(out, "r[%d]", reg->name);
		break;
	case LABEL:
		fprintf(out, "instr_%d", jump->name);
		break;
	case FUNC:
		fprintf(out, "func_%lld", value_const);
		break;
	default:
		assert(false);
	}
}

Instruction::Instruction (FILE *in)
{
	char buf[201];
	int ret;

	ret = fscanf(in, " instr %d:", &name);
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
	if (op == Opcode::CALL) {
		assert(oper[0].type == Operand::LABEL);
		oper[0].type = Operand::FUNC;
	}
}

void Instruction::icode (FILE *out) const
{
	fprintf(out, "instr %d: %s", name, op.name());
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
	fprintf(out, "instr_%d: ", name);
	switch(op) {
	case Opcode::ADD:
		fprintf(out, "r[%d] = ", name); oper[0].ccode(out); fprintf(out, " + "); oper[1].ccode(out);
		break;
	case Opcode::SUB:
		fprintf(out, "r[%d] = ", name); oper[0].ccode(out); fprintf(out, " - "); oper[1].ccode(out);
		break;
	case Opcode::MUL:
		fprintf(out, "r[%d] = ", name); oper[0].ccode(out); fprintf(out, " * "); oper[1].ccode(out);
		break;
	case Opcode::DIV:
		fprintf(out, "r[%d] = ", name); oper[0].ccode(out); fprintf(out, " / "); oper[1].ccode(out);
		break;
	case Opcode::MOD:
		fprintf(out, "r[%d] = ", name); oper[0].ccode(out); fprintf(out, " %% "); oper[1].ccode(out);
		break;
	case Opcode::NEG:
		fprintf(out, "r[%d] = ", name); fprintf(out, "- "); oper[0].ccode(out);
		break;
	case Opcode::CMPEQ:
		fprintf(out, "r[%d] = ", name); oper[0].ccode(out); fprintf(out, " == "); oper[1].ccode(out);
		break;
	case Opcode::CMPLE:
		fprintf(out, "r[%d] = ", name); oper[0].ccode(out); fprintf(out, " <= "); oper[1].ccode(out);
		break;
	case Opcode::CMPLT:
		fprintf(out, "r[%d] = ", name); oper[0].ccode(out); fprintf(out, " < "); oper[1].ccode(out);
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
		fprintf(out, "SP -= 8; MEM(SP) = %d + 1; ", name); oper[0].ccode(out); fprintf(out, "()");
		break;
	case Opcode::LOAD:
		fprintf(out, "r[%d] = MEM(", name); oper[0].ccode(out); fprintf(out, ")");
		break;
	case Opcode::STORE:
		fprintf(out, "MEM("); oper[1].ccode(out); fprintf(out, ") = "); oper[0].ccode(out);
		break;
	case Opcode::MOVE:
		fprintf(out, "r[%d] = ", name); oper[1].ccode(out); fprintf(out, " = "); oper[0].ccode(out);
		break;
	case Opcode::READ:
		fprintf(out, "ReadLong(r[%d])", name);
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
}

int Instruction::get_branch_oper () const
{
	if (op.type == Opcode::BR)
		return 0;
	if (op.type == Opcode::BLBC || op.type == Opcode::BLBS)
		return 1;
	return -1;
}

Block *Instruction::get_branch_target () const
{
	if (op.type == Opcode::BR)
		return oper[0].jump;
	if (op.type == Opcode::BLBC || op.type == Opcode::BLBS)
		return oper[1].jump;
	return NULL;
}

void Instruction::set_branch(Block *block)
{
    if (op.type == Opcode::BR)
        oper[0].jump = block;
    else if (op.type == Opcode::BLBC || op.type == Opcode::BLBS)
        oper[1].jump = block;
}

bool Instruction::falldown() const
{
	if (op == Opcode::BR || op == Opcode::RET)
		return false;
	return true;
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
		return oper[0].value_const + oper[1].value_const;
	case Opcode::SUB:
		return oper[0].value_const - oper[1].value_const;
	case Opcode::MUL:
		return oper[0].value_const * oper[1].value_const;
	case Opcode::DIV:
		return oper[0].value_const / oper[1].value_const;
	case Opcode::MOD:
		return oper[0].value_const % oper[1].value_const;
	case Opcode::CMPEQ:
		return oper[0].value_const == oper[1].value_const;
	case Opcode::CMPLE:
		return oper[0].value_const <= oper[1].value_const;
	case Opcode::CMPLT:
		return oper[0].value_const < oper[1].value_const;
	case Opcode::NEG:
		return -oper[0].value_const;
	case Opcode::MOVE:
		return oper[0].value_const;
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

void Instruction::erase()
{
    op.type = Opcode::NOP;
    oper[0].type = Operand::UNKNOWN;
    oper[1].type = Operand::UNKNOWN;
}

Program::Program (FILE *in):
	Program()
{
	std::vector<Instruction> instr;

	while (!feof(in)) {
		instr.push_back(Instruction(in));
	}
	if (!instr.back())
		instr.pop_back();

	for (auto p = instr.begin(), enter = instr.end(); p != instr.end(); ++p) {
		if (p->op == Opcode::ENTER) {
			assert(enter == instr.end());
			enter = p;
		} else if (p->op == Opcode::RET) {
			assert(enter != instr.end());
			Function *f = new Function(this, enter, p+1);
			funcs.push_back(f);
			if (enter != instr.begin() && (--enter)->op == Opcode::ENTRYPC) {
				assert(main == NULL);
				main = f;
			}
			enter = instr.end();
		}
	}
	assert(main != NULL);
}

void Program::rename()
{
	int i = 2;
	for (Function *func: funcs) {
		if (func == main)
			++i;
		i = func->rename(i);
	}
}

void Program::icode (FILE *out)
{
	ssa_mode = false;
	fprintf(out, "instr 1: nop\n");
	int i;

	for (Function* func : funcs) {
		if (func == main)
			fprintf(out, "instr %d: entrypc\n", func->entry->name - 1);

		i = func->icode(out);
	}

	fprintf(out, "instr %d: nop\n", i);
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
	fprintf(out, "void func_%d();\nvoid (*entry)() = func_%d;\n\n", main->name, main->name);

	for (Function* func : funcs)
		func->ccode(out);

	fprintf(out,"void main()\n{\n\t(*entry)();\n}\n");
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

void Operand::to_const(long long val)
{
    type = Operand::CONST;
    value_const = val;
    tag.clear();
    ssa_idx = -1;
}

Program::~Program()
{
    for (Function* func : funcs)
        delete func;
}

Function::Function(Program *parent, std::vector<Instruction>::iterator begin, std::vector<Instruction>::iterator end)
    : prog(parent)
{
	auto exit = end;
	--exit;
	assert(begin->op == Opcode::ENTER);
	assert(exit->op == Opcode::RET);
	frame_size = begin->oper[0]._value/ 8;
	arg_count = exit->oper[0]._value/ 8;

	std::map<int, int> addr_instr;
	std::vector<Instruction*> instr;
	for (auto p = begin; p != end; ++p) {
		assert(addr_instr.count(p->name) == 0);
		addr_instr[p->name] = instr.size();
		instr.push_back(new Instruction(*p));
	}

	std::set<int> bounds;  // boundaries of blocks
	for (int i = 0; i < (int)instr.size(); ++i) {
		int o = instr[i]->get_branch_oper();
		if (o != -1 || instr[i]->op == Opcode::CALL)
			bounds.insert(i + 1);
		if (o != -1)
			bounds.insert(addr_instr[instr[i]->oper[o]._value]);
	}
	bounds.insert(instr.size());

	std::map<int, Block*> addr_blocks;
	int st = 0;
	for (int ed : bounds) {
		auto it = instr.begin();
		Block *b = new Block(this, it + st, it + ed);
		addr_blocks[b->name] = b;
		blocks.push_back(b);
		st = ed;
	}
	entry = addr_blocks[instr[0]->name];
	name = entry->name;

	std::map<int, Localvar*> vars;
	for (Instruction *i: instr) for (int o = 0; o < 2; ++o) {
		Operand &oper = i->oper[o];
		switch (oper) {
		case Operand::CONST:
			oper.value_const = oper._value;
			break;
		case Operand::LOCAL:
			if (vars.count(oper._value) == 0) {
				Localvar *var = new Localvar(oper.tag, oper._value);
				vars[oper._value] = var;
				localvars.push_back(var);
				oper.var = var;
				oper.tag.clear();
			} else {
				assert(oper.tag == vars[oper._value]->name);
				oper.var = vars[oper._value];
				oper.tag.clear();
			}
			break;
		case Operand::REG:
			oper.reg = instr[addr_instr[oper._value]];
			break;
		case Operand::LABEL:
			oper.jump = addr_blocks[oper._value];
			break;
		case Operand::FUNC:
			oper.value_const = oper._value;
			break;
		}
	}

	st = 0;
	for (int ed : bounds) {
		Block *b = addr_blocks[instr[st]->name];

		Instruction *i = b->instr.back();
		b->seq_next = i->falldown() ? addr_blocks[instr[ed]->name] : nullptr;
		if (b->seq_next)
			b->seq_next->prevs.push_back(b);
		b->order_next = (i->op != Opcode::RET) ? addr_blocks[instr[ed]->name] : nullptr;

		b->br_next = b->instr.back()->get_branch_target();
		if (b->br_next) {
			assert(b->br_next != b->seq_next);
			b->br_next->prevs.push_back(b);
		}

		st = ed;
	}
}

Function::~Function()
{
    for (auto iter : blocks)
        delete iter;
    for (auto iter : localvars)
        delete iter;
}

int Function::rename(int i)
{
	for (Block *p = entry; p != NULL; p = p->order_next) {
		assert(!p->instr.empty());
		p->name = i;
		for (Instruction *ins: p->instr) if (ins->op != Opcode::NOP) {
			ins->name = i++;
		}
	}
	// Function name not change
	//name = entry->name;
	return i;
}

void Function::ccode(FILE *out) const
{
	fprintf(out, "void func_%d() {\n", name);

	for (Block *p = entry; p != NULL; p = p->order_next) {
		assert(!p->instr.empty());
		for (Instruction *ins: p->instr) if (ins->op != Opcode::NOP)
			ins->ccode(out);
	}

	fprintf(out, "}\n");
}

int Function::icode(FILE *out) const
{
	int i;
	for (Block *p = entry; p != NULL; p = p->order_next) {
		assert(!p->instr.empty());
		for (Instruction *ins: p->instr) if (ins->op != Opcode::NOP)
			ins->icode(out);
		i = p->instr.back()->name;
	}
	return i+1;
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
		for (Block *b: blocks) {
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
	std::multimap<Block*, Block*> backedge;
	for (Block *b: blocks) {
		blockset &bs = dominators.find(b)->second;
		if (b->seq_next != NULL && bs.find(b->seq_next) != bs.end())
			backedge.insert(std::make_pair(b->seq_next, b));
		if (b->br_next != NULL && bs.find(b->br_next) != bs.end())
			backedge.insert(std::make_pair(b->br_next, b));
	}
	for (Block *b: blocks) {
		auto st = backedge.lower_bound(b), ed = backedge.upper_bound(b);
		if (st != ed) {
			loops[b] = blockset();
			blockset candid, &loop = loops[b];
			loop.insert(b);
			for (auto i = st; i != ed; ++i) {
				candid.insert(i->second);
				loop.insert(i->second);
			}
			while (!candid.empty()) {
				Block *s = *candid.begin();
				candid.erase(candid.begin());
				for (Block *t: s->prevs) if (loop.find(t) == loop.end()) {
					candid.insert(t);
					loop.insert(t);
				}
			}
		}
	}
	for (Block *b: blocks) {
		b->idom = NULL;
		b->domc.clear();
	}
	for (Block *b: blocks) {
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
	// uintptr-t -> Instruction*
	typedef std::set<std::pair<Localvar*, uintptr_t> > iset; // Set of variable definitions
	int propa_count = 0;
	std::map<Block*, iset> rd; // Reaching Definition IN
	for (Block *b: blocks) {
		rd[b] = iset();
	}
	for (Localvar *v: localvars) if (v->offset > 0)
		// Insert function agruments
		// Instruction after enter
		rd[entry].insert(std::make_pair(v, (uintptr_t)entry->instr.front()));
	bool change;
	do {
		change = false;
		for (Block *b: blocks) {
			iset cur = rd[b]; // Copy
			for (Instruction *i: b->instr) {
				if (i->op == Opcode::MOVE && i->oper[1] == Operand::LOCAL) {
					Localvar *def = i->oper[1].var;
					auto vst = cur.lower_bound(std::make_pair(def, 0));
					auto ved = cur.lower_bound(std::make_pair(def, UINTPTR_MAX));
					cur.erase(vst, ved);
					cur.insert(std::make_pair(def, (uintptr_t)i));
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
		for (Block *b: blocks) {
			iset cur = rd[b]; // Copy
			for (Instruction *i: b->instr) {
				for (int o = 0; o < 2; ++o) if (i->isrightvalue(o)) switch(i->oper[o].type) {
				case Operand::LOCAL: {
					Localvar *var = i->oper[o].var;
					auto vst = cur.lower_bound(std::make_pair(var, 0));
					auto ved = cur.lower_bound(std::make_pair(var, UINTPTR_MAX));
					bool flag = true;
					long long value = 0;
					for (auto v = vst; v != ved; ++v) {
						Instruction *ins2 = (Instruction *)v->second;
						assert(ins2->op == Opcode::MOVE || ins2->op == Opcode::ENTER);
						if (ins2->op == Opcode::MOVE && ins2->oper[0].type == Operand::CONST) {
							if (v == vst) {
								value = ins2->oper[0].value_const;
							} else if (value != ins2->oper[0].value_const) {
								flag = false;
							}
						} else {
							flag = false;
						}
						if (!flag)
							break;
					}
					if (flag) {
						i->oper[o].to_const(value);
						change = true;
						propa_count++;
					}
					break;
				}
				case Operand::REG: {
					Instruction *ins2 = i->oper[o].reg;
					if (ins2->isconst()) {
						i->oper[o].to_const(ins2->constvalue());
						change = true;
						propa_count++;
					}
				}
				}
				if (i->op == Opcode::MOVE && i->oper[1] == Operand::LOCAL) {
					Localvar *def = i->oper[1].var;
					auto vst = cur.lower_bound(std::make_pair(def, 0));
					auto ved = cur.lower_bound(std::make_pair(def, UINTPTR_MAX));
					cur.erase(vst, ved);
					cur.insert(std::make_pair(def, (uintptr_t)i));
				}
			}
		}
	} while(change);
	if (output_report) {
		printf("Function: %d\n", name);
		printf("Number of constants propagated: %d\n", propa_count);
	}
}

void Function::dead_eliminate()
{
	typedef std::set<Localvar*> iset; // Set of variable definitions
	int elimin_count_in = 0, elimin_count_out = 0;
	std::map<Block*, iset> lv; // Live Variables OUT
	std::set<Instruction*> lvreg; // Live Temporary Registers
	std::set<Block*> inloop; // Block in Loops
	for (Block *b: blocks) {
		lv[b] = iset();
	}
	for (auto s: loops) for (Block *b: s.second) {
		inloop.insert(b);
	}
	bool change;
	do {
		change = false;
		for (auto ib = blocks.rbegin(); ib != blocks.rend(); ++ib) {
			Block *b = *ib;
			iset cur = lv[b]; // Copy
			for (auto j = b->instr.rbegin(); j != b->instr.rend(); ++j) {
				Instruction *i = *j;
				bool live = false;
				if (!i->eliminable() || lvreg.find(i) != lvreg.end()) {
					live = true;
				}
				if (i->op == Opcode::MOVE && i->oper[1] == Operand::LOCAL) {
					Localvar *def = i->oper[1].var;
					if (cur.find(def) != cur.end()) {
						cur.erase(def);
						live = true;
					}
				}
				if (live) for (int o = 0; o < 2; ++o) if (i->isrightvalue(o)) {
					switch(i->oper[o]) {
					case Operand::LOCAL: {
						Localvar *use = i->oper[o].var;
						cur.insert(use);
						break;
					}
					case Operand::REG: {
						Instruction *reg = i->oper[o].reg;
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
		Block *b = *ib;
		iset cur = lv[b]; // Copy
		bool inl = (inloop.find(b) != inloop.end());
		for (auto j = b->instr.rbegin(); j != b->instr.rend(); ++j) {
			Instruction *i = *j;
			bool live = false;
			if (!i->eliminable() || lvreg.find(i) != lvreg.end()) {
				live = true;
			}
			if (i->op == Opcode::MOVE && i->oper[1] == Operand::LOCAL) {
				Localvar *def = i->oper[1].var;
				if (cur.find(def) != cur.end()) {
					cur.erase(def);
					live = true;
				}
			}
			if (live) {
				for (int o = 0; o < 2; ++o) if (i->isrightvalue(o)) {
					switch(i->oper[o]) {
					case Operand::LOCAL: {
						Localvar *use = i->oper[o].var;
						cur.insert(use);
						break;
					}
					}
				}
			} else {
				/* Eliminate */
				int name = i->name;
				*i = Instruction();
				i->op.type = Opcode::NOP;
				i->name = name;
				if (inl)
					++elimin_count_in;
				else
					++elimin_count_out;
			}
		}
	}
	if (output_report) {
		fprintf(stderr, "Function: %d\n", name);
		fprintf(stderr, "Number of statements eliminated in SCR: %d\n", elimin_count_in);
		fprintf(stderr, "Number of statements eliminated not in SCR: %d\n", elimin_count_out);
	}
}

Block::~Block() {
    for (auto iter : instr)
        delete iter;
}
