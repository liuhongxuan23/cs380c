
#include <cassert>
#include <string>
#include <vector>
#include <list>
#include <stack>
#include <map>
#include <set>
#include <unordered_set>
#include <unordered_map>

extern bool output_report;

struct Instruction;
struct Block;
struct Function;
struct Localvar;

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
		UNKNOWN = 0, GP, FP, CONST, LOCAL, REG, LABEL, FUNC
	};
	Type type;
	union {
		long long _value;
		long long value_const;
		Instruction *reg;
		Block *jump;
		Localvar *var;
	};
	std::string tag;
	Operand (): type(UNKNOWN), _value(0), tag() {}
	Operand (const char *str);
	operator Type() const { return type; }
	void icode (FILE *out) const;
	void ccode (FILE *out) const;

        // SSA
        int ssa_idx = -1;

        bool is_local() const { return type == LOCAL; }
        bool is_const() const { return type == CONST; }
        void to_const(long long val);

        bool operator< (const Operand& o) const;
};

struct Instruction {
	int name = -1;
	Opcode op;
	Operand oper[2];
	Instruction () {}
	Instruction (FILE *in);
	void icode (FILE *out) const;
	void ccode (FILE *out) const;
	operator bool() const { return bool(op); }
        int get_branch_oper () const;
        Block *get_branch_target () const;
	bool falldown() const;
        void set_branch(Block *block);
	bool isconst() const;
	long long constvalue() const;
	bool isrightvalue(int o) const;
	bool eliminable() const;

        void erase();
        bool is_move() const { return op == Opcode::MOVE; }
};

class Function;
class Program;

struct Phi {
    int name = 0;
    int l;
    std::vector<Operand> r;
    std::vector<Block*> pre;

    void init(std::vector<Block*>& p) { pre = p; r.resize(p.size()); }
    void clear() { r.clear(); pre.clear(); }
    long long value() const { return r[0].value_const; }
    bool is_const() const;
    void icode(FILE* out, Localvar* var) const;
    bool empty() const { return r.empty(); }
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
    Block(Function* func, std::vector<Instruction*>::iterator begin, std::vector<Instruction*>::iterator end)
        : func(func), instr(begin, end), name((*begin)->name) {}
    ~Block();

    Function* func;
    int name;
    Block* seq_next = nullptr;
    Block* br_next = nullptr;
    Block* order_next = nullptr;
    std::list<Instruction*> instr;
    std::vector<Block*> prevs;
    std::vector<Block*> prevs2;
    Block *idom = NULL;
    std::list<Block*> domc;

    std::vector<Instruction*> insert;

    long long addr() const {
	/* TODO */
	return name;
    }

    // SSA
    std::vector<Block*> df;
    std::unordered_set<Localvar*> defs;
    std::unordered_map<Localvar*, Phi> phi;
    long long ssa_addr = 0;

    void compute_df();
    void find_defs();
    void ssa_rename_var(std::map<Localvar*, RenameStack>& stack);

    void append(Instruction* in);
};

struct Localvar {
	std::string name;
	long long offset;
	Localvar () {}
	Localvar (std::string n, long long o): name(n), offset(o) {}
};

class Function {
public:
    Function(Program* parent, std::vector<Instruction>::iterator begin, std::vector<Instruction>::iterator end);
    ~Function();
    int rename (int i);
    int icode (FILE *out) const;
    void ccode (FILE *out) const;

    Program* prog;
    int name;
    int frame_size;
    int arg_count;
    bool is_main;
    std::vector<Localvar*> localvars;
    std::vector<Block*> blocks;
    std::map<Block*, std::set<Block*> > loops;
    Block* entry;

    void build_domtree();
    void constant_propagate();
    void dead_eliminate();

    // SSA
    std::unordered_map<int, std::string> offset2tag;
    void place_phi();
    void remove_phi();
    void ssa_constant_propagate();
    void ssa_licm();
};

struct Program {
        std::vector<Function*> funcs;
	Function *main = NULL;
        Program () {}
	Program (FILE *in);
        ~Program ();
	void rename ();
	void icode (FILE *out);
	void ccode (FILE *out);
	void build_domtree();
	void constant_propagate();
	void dead_eliminate();

        // SSA
        int instr_cnt = 0;

        void ssa_prepare();

        void compute_df();
        void find_defs();
        void place_phi();
        void remove_phi();
        void ssa_rename_var();
        void ssa_licm();
        void ssa_constant_propagate();
        void ssa_to_3addr();

        void ssa_icode(FILE* out);

        static bool ssa_mode;
};
