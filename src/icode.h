
#include <string>
#include <vector>
#include <list>
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
	operator Type() const { return type; }

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
	operator Type() const { return type; }
	void icode (FILE *out);
	void ccode (FILE *out);
};

struct Instruction {
	long long addr;
	Opcode op;
	Operand oper[2];
	Instruction (): addr(-1) {}
	Instruction (FILE *in);
	void icode (FILE *out);
	void ccode (FILE *out);
	operator bool() { return bool(op); }
        int get_branch_target () const;
	int get_next_instr() const;
	bool isconst() const;
	long long constvalue() const;
	bool isrightvalue(int o) const;
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
    std::list<Block*> prevs;
    Block *domf = NULL;
    std::list<Block*> domc;
};

class Function {
public:
    Function(Program* parent, int enter, int exit);
    ~Function();

    Program* prog;
    int frame_size;
    bool is_main;
    std::map<int, Block*> blocks;
    Block* entry;

    void build_domtree();
};

struct Program {
	std::vector<Instruction> instr;
        std::vector<Function*> funcs;
        Program () { instr.push_back(Instruction()); }
	Program (FILE *in);
        ~Program ();
	void icode (FILE *out);
	void ccode (FILE *out);
        void find_functions();
	void build_domtree();
	void constant_propagate();
};
