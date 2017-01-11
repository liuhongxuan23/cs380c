
#include <string>
#include <vector>
#include <map>

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
	void ccode (FILE *out);
};

struct Instruction {
	long long addr;
	Opcode op;
	Operand oper1;
	Operand oper2;
	Instruction (): addr(-1) {}
	Instruction (FILE *in);
	void icode (FILE *out);
	void ccode (FILE *out);
	operator bool() { return bool(op); }
        int get_branch_target () const;
};

class Function;
class Program;

class Block {
public:
    Block(Function* func, std::vector<Instruction> instr);

    Function* func;
    Block* seq_next = nullptr;
    Block* br_next = nullptr;
    std::vector<Instruction> instr;
};

class Function {
public:
    Function(Program* parent, int enter, int exit);
    ~Function();

    Program* prog;
    int frame_size;
    bool is_main;
    std::map<int, Block*> blocks;

private:
    void find_basic_blocks();
};

struct Program {
	std::vector<Instruction> instr;
        std::vector<Function*> funcs;
        Program () { instr.push_back(Instruction()); }
	Program (FILE *in);
        ~Program ();
	void icode (FILE *out);
	void ccode (FILE *out);

private:
        void find_functions();
};
