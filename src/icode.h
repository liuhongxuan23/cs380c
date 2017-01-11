
#include <string>
#include <vector>

struct Opcode {
	enum Type {
		UNKNOWN = 0,
		ADD, SUB, MUL, DIV, MOD, NEG, CMPEQ, CMPLE, CMPLT,
		NOP,
		BR, BLBC, BLBS, CALL,
		LOAD, STORE, MOVE,
		READ, WRITE, WRL,
		PARAM,
		ENTER,
		RET,
		ENTRYPC,
		OPCODE_MAX,
	};
	static const char *const opname[OPCODE_MAX];
	static const int operand_count[OPCODE_MAX];

	Type type;

	Opcode (): type(UNKNOWN) {}
	Opcode (const char *name);
	operator Type() { return type; }

	const char *name() const { return opname[type]; }
	int operands() const { return operand_count[type]; }
};


struct Operand {
	enum Type {
		UNKNOWN = 0, GP, FP, CONST, LOCAL, REG, LABEL,
	};
	Type type;
	long long value;
	std::string tag;
	Operand (): type(UNKNOWN), value(0), tag() {}
	Operand (const char *str);
	operator bool() { return type != UNKNOWN; }
	void icode (FILE *out);
};

struct Instruction {
	long long addr;
	Opcode op;
	Operand oper1;
	Operand oper2;
	Instruction (): addr(-1) {}
	Instruction (FILE *in);
	void icode (FILE *out);
	operator bool() { return bool(op); }
};

struct Program {
	std::vector<Instruction> instr;
	Program () {}
	Program (FILE *in);
	void icode (FILE *out);
};;
