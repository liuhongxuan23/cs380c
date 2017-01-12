#include "icode.h"

#include <cassert>
#include <algorithm>

using std::string;
using std::vector;
using std::map;
using std::unordered_set;
using std::unordered_map;

static inline bool is_ancestor(Block* x, Block* y)
{
    while (y != x && y != nullptr)
        y = y->idom;
    return y == x;
}

void Block::compute_df()
{
    unordered_set<Block*> df_set;

    if (seq_next && seq_next->idom != this)
        df_set.insert(seq_next);
    if (br_next && br_next->idom != this)
        df_set.insert(br_next);

    for (Block* child : domc) {
        child->compute_df();
        for (Block* child_df : child->df)
            if (this == child_df || !is_ancestor(this, child_df))
                df_set.insert(child_df);
    }

    df = vector<Block*>(df_set.cbegin(), df_set.cend());
}

void Block::find_defs()
{
    for (Instruction& in : instr)
        if (in.op == Opcode::MOVE && !in.oper[1].tag.empty())
            defs.insert(in.oper[1].tag);
}

void Function::place_phi()
{
    unordered_map<string, vector<Block*>> def_sites;

    for (const auto& addr_block : blocks)
        for (const string& var : addr_block.second->defs)
            def_sites[var].push_back(addr_block.second);

    for (auto& var_blocks : def_sites) {
        string var = var_blocks.first;
        auto& blocks = var_blocks.second;

        while (!blocks.empty()) {
            Block* b = blocks.back();
            blocks.pop_back();

            for (Block* df : b->df)
                if (df->phi.count(var) == 0) {
                    df->phi[var].r.resize(df->prevs.size());
                    if (df->defs.count(var) == 0)
                        blocks.push_back(df);
                }
        }
    }
}

static void rename_phi(Block* child, Block* parent, map<string, RenameStack>& stack)
{
    if (child == nullptr) return;

    auto it = std::find(child->prevs.cbegin(), child->prevs.cend(), parent);
    int p = it - child->prevs.cbegin();

    for (auto& var_phi : child->phi)
        var_phi.second.r[p] = stack[var_phi.first].top();
}

void Block::ssa_rename_var(map<string, RenameStack>& stack)
{
    for (auto& var_phi : phi)
        var_phi.second.l = stack[var_phi.first].push();

    for (auto& in : instr) {
        if (!in.oper[0].tag.empty())
            in.oper[0].ssa_idx = stack[in.oper[0].tag].top();
        if (!in.oper[1].tag.empty())
            in.oper[1].ssa_idx = stack[in.oper[1].tag].top();

        if (in.op == Opcode::MOVE && !in.oper[1].tag.empty())
            in.oper[1].ssa_idx = stack[in.oper[1].tag].push();
    }

    rename_phi(seq_next, this, stack);
    rename_phi(br_next, this, stack);

    for (Block* c : domc)
        c->ssa_rename_var(stack);

    for (const string& var : defs)
        stack[var].pop();
}

void Program::ssa_icode(FILE* out)
{
    fputs("instr 1: nop\n", out);

    int n = instr.size();

    for (Function* func : funcs) {
        if (func->is_main)
            fprintf(out, "instr %lld: entrypc\n", func->entry->addr() - 1);

        for (auto& addr_block : func->blocks) {
            auto& block = addr_block.second;
            block->ssa_addr = block->phi.empty() ? block->addr() : n;
            n += block->phi.size() * 2;
        }

        for (auto& addr_block : func->blocks) {
            auto& block = addr_block.second;
            long long addr = block->ssa_addr;

            for (auto& var_phi : block->phi) {
                const string& var = var_phi.first;
                auto& phi = var_phi.second;

                fprintf(out, "instr %lld: phi", addr);
                for (int idx : phi.r)
                    fprintf(out, " %s$%d", var.c_str(), idx);
                fputc('\n', out);

                fprintf(out, "instr %lld: move (%lld) %s$%d\n", addr + 1, addr, var.c_str(), phi.l);

                addr += 2;
            }

            for (int i = 0; i < block->instr.size() - 1; ++i)
                block->instr[i].icode(out);

            Instruction& in = block->instr.back();
            if (in.op == Opcode::BR)
                fprintf(out, "instr %lld: br [%lld]\n", in.addr, block->br_next->ssa_addr);
            else if (in.op == Opcode::BLBC)
                fprintf(out, "instr %lld: blbc (%lld) [%lld]\n", in.addr, in.oper[0].value, block->br_next->ssa_addr);
            else if (in.op == Opcode::BLBS)
                fprintf(out, "instr %lld: blbs (%lld) [%lld]\n", in.addr, in.oper[0].value, block->br_next->ssa_addr);
            else
                in.icode(out);
        }
    }

    fprintf(out, "instr %d: nop\n", (int)instr.size() - 1);
}

void Program::compute_df()
{
    for (Function* func : funcs)
        func->entry->compute_df();
}

void Program::find_defs()
{
    for (Function* func : funcs)
        for (const auto& addr_block : func->blocks)
            addr_block.second->find_defs();
}

void Program::place_phi()
{
    for (Function* func : funcs)
        func->place_phi();
}

void Program::ssa_rename_var()
{
    for (Function* func : funcs) {
        map<string, RenameStack> stack;
        func->entry->ssa_rename_var(stack);
    }
}
