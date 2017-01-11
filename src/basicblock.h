#include "icode.h"

#include <map>

struct BasicBlock {
    int begin;
    int end;

    static std::map<int, BasicBlock*> blocks;

    BasicBlock(int begin, int end = -1) : begin(begin), end(end) { blocks[begin] = this; }

    BasicBlock* get_seq_next() const;
    BasicBlock* get_br_next() const;
};

class Function {
public:
    int begin;
    int end;

    int frame_size = 0;
    bool is_main = false;

    static std::vector<Function*> funcs;

    Function(int begin, int end = -1) : begin(begin), end(end) { funcs.push_back(this); }

    static void init(const Program* prog);

private:
    void find_basic_blocks();

    BasicBlock* entry;
};
