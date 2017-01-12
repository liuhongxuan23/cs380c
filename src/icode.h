
#include <string>
#include <vector>
#include <list>
#include <stack>
#include <map>
#include <unordered_set>
#include <unordered_map>

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
	void icode (FILE *out) const;
	void ccode (FILE *out) const;

        // SSA
        int ssa_idx = -1;
};

struct Instruction {
	long long addr;
	Opcode op;
	Operand oper[2];
	Instruction (): addr(-1) {}
	Instruction (FILE *in);
	void icode (FILE *out) const;
	void ccode (FILE *out) const;
	operator bool() const { return bool(op); }
        int get_branch_target () const;
	int get_next_instr() const;
	bool isconst() const;
	long long constvalue() const;
	bool isrightvalue(int o) const;
};

class Function;
class Program;

struct Phi {
    int l;
    std::vector<int> r;
};

struct RenameStack {
    int cnt;
    std::stack<int> stack;

    RenameStack() : cnt(0) { stack.push(0); }
    int push() { stack.push(++cnt); return cnt; }
    int pop() { int r = stack.top(); stack.pop(); return r; }
    int top() { return stack.top(); }
};

class Block {
public:
    Block(Function* func, std::vector<Instruction> instr);

    Function* func;
    Block* seq_next = nullptr;
    Block* br_next = nullptr;
    std::vector<Instruction> instr;
    std::vector<Block*> prevs;
    Block *idom = NULL;
    std::list<Block*> domc;

    long long addr() const { return instr[0].addr; }

    // SSA
    std::vector<Block*> df;
    std::unordered_set<std::string> defs;
    std::unordered_map<std::string, Phi> phi;
    long long ssa_addr = 0;

    void compute_df();
    void find_defs();
    void ssa_rename_var(std::map<std::string, RenameStack>& stack);
};

class Function {
public:
    Function(Program* parent, int a, int b);
    ~Function();

    Program* prog;
    int enter;
    int exit;
    int frame_size;
    int arg_count;
    bool is_main;
    std::map<int, Block*> blocks;
    Block* entry;

    void build_domtree();
    void constant_propagate();

    // SSA
    void place_phi();
};

struct Program {
	std::vector<Instruction> instr;
        std::vector<Function*> funcs;
        Program () { instr.push_back(Instruction()); }
	Program (FILE *in);
        ~Program ();
	void icode (FILE *out) const;
	void ccode (FILE *out) const;
        void find_functions();
	void build_domtree();
	void constant_propagate();

        // SSA
        void compute_df();
        void find_defs();
        void place_phi();
        void ssa_rename_var();

        void ssa_icode(FILE* out);
};
