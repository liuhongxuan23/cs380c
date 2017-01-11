#include "basicblock.h"

#include <cassert>

static const Program *prog;

std::map<int, BasicBlock*> BasicBlock::blocks;
std::vector<Function*> Function::funcs;

BasicBlock* BasicBlock::get_seq_next() const
{
    int op = prog->instr[end - 1].op.type;
    if (op == Opcode::BR || op == Opcode::RET)
        return nullptr;
    assert(blocks[end] != nullptr);
    return blocks[end];
}

BasicBlock* BasicBlock::get_br_next() const
{
    int line = prog->instr[end - 1].get_branch_target();
    if (line <= 0)
        return nullptr;
    assert(blocks[end] != nullptr);
}

void Function::init(const Program* prog_)
{
    prog = prog_;

    Function* cur = nullptr;

    for (int i = 1; i < prog->instr.size(); ++i) {
        int op = prog->instr[i].op.type;

        if (op == Opcode::ENTER) {
            assert(cur == nullptr);
            cur = new Function(i + 1);
            cur->frame_size = prog->instr[i].oper1.value;
            cur->is_main = (prog->instr[i - 1].op.type == Opcode::ENTRYPC);

        } else if (op == Opcode::RET) {
            assert(cur != nullptr);
            cur->end = i + 1;
            cur->find_basic_blocks();
            cur = nullptr;
        }
    }

    auto& blocks = BasicBlock::blocks;
    auto iter = blocks.begin();
    while (iter != blocks.end()) {
        BasicBlock* block = iter->second;
        int line = prog->instr[block->end - 1].get_branch_target();

        if (line > 0 && blocks.count(line) == 0) {  // target is inside a basic block, split it
            BasicBlock* split = std::prev(blocks.upper_bound(line))->second;
            new BasicBlock(line, split->end);
            split->end = line;
        }

        iter = blocks.lower_bound(block->end);
    }
}

void Function::find_basic_blocks()
{
    entry = new BasicBlock(begin);
    BasicBlock* cur = entry;

    for (int i = begin + 1; i < end - 1; ++i) {
        if (prog->instr[i].get_branch_target() != -1) {
            cur->end = i + 1;
            cur = new BasicBlock(i + 1);
        }
    }

    cur->end = end;
}
