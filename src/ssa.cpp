#include "icode.h"

#include <cassert>
#include <algorithm>
#include <unordered_map>

using std::string;
using std::vector;
using std::map;
using std::multimap;
using std::unordered_set;
using std::unordered_map;
using std::pair;
using std::make_pair;

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
    for (Instruction* in : instr)
        if (in->is_move() && in->oper[1].is_local())
            defs.insert(in->oper[1].var);
}

void Function::place_phi()
{
    unordered_map<Localvar*, vector<Block*>> def_sites;

    for (Block* b : blocks)
        for (Localvar* var : b->defs)
            def_sites[var].push_back(b);

    for (auto& var_blocks : def_sites) {
        Localvar* var = var_blocks.first;
        auto& blocks = var_blocks.second;

        while (!blocks.empty()) {
            Block* b = blocks.back();
            blocks.pop_back();

            for (Block* df : b->df)
                if (df->phi.count(var) == 0) {
                    df->phi[var].init(df->prevs);
                    if (df->defs.count(var) == 0)
                        blocks.push_back(df);
                }
        }
    }
}

static Operand ssa_oper(Localvar* var, int idx)
{
    Operand r;
    r.type = Operand::LOCAL;
    r.var = var;
    r.ssa_idx = idx;
    return r;
}

static void rename_phi(Block* child, Block* parent, map<Localvar*, RenameStack>& stack)
{
    if (child == nullptr) return;

    auto it = std::find(child->prevs.cbegin(), child->prevs.cend(), parent);
    int p = it - child->prevs.cbegin();

    for (auto& var_phi : child->phi) {
        Localvar* var = var_phi.first;
        Phi& phi = var_phi.second;
        phi.r[p] = ssa_oper(var, stack[var].top());
        phi.pre[p] = parent;
    }
}

static Operand reg_oper(Instruction* in)
{
    Operand r;
    r.type = Operand::REG;
    r.reg = in;
    r.ssa_idx = -1;
    return r;
}

void Block::ssa_rename_var(map<Localvar*, RenameStack>& stack)
{
    for (auto& var_phi : phi)
        var_phi.second.l = stack[var_phi.first].push();

    for (Instruction* in : instr) {
        if (in->oper[0].is_local())
            in->oper[0].ssa_idx = stack[in->oper[0].var].top();
        if (in->oper[1].is_local())
            in->oper[1].ssa_idx = stack[in->oper[1].var].top();

        if (in->is_move() && in->oper[1].is_local())
            in->oper[1].ssa_idx = stack[in->oper[1].var].push();
    }

    assert(seq_next == nullptr || seq_next != br_next);
    rename_phi(seq_next, this, stack);
    rename_phi(br_next, this, stack);

    for (Block* c : domc)
        c->ssa_rename_var(stack);

    for (Localvar* var : defs)
        stack[var].pop();
}


/*
void Function::remove_phi()
{
    Instruction bak;
    bool addr_updated;

    for (auto& addr_block : blocks) {
        Block* b = addr_block.second;

        for (int i = 0; i < b->prevs.size(); ++i) {
            Block* prev = b->prevs[i];
            if (prev->br_next == b) {
                bak = prev->instr.back();
                prev->instr.pop_back();
                addr_updated = prev->instr.empty();

            } else assert(prev->seq_next == b);

            for (auto var_phi : b->phi) {
                int var = var_phi.first;
                Phi& phi = var_phi.second;
                if (phi.r.empty()) continue;

                if (phi.r[i].type == Operand::CONST) {
                    Instruction in;
                    in.addr = prog->instr_cnt++;
                    in.op.type = Opcode::MOVE;
                    in.oper[0].type = Operand::CONST;
                    in.oper[0].value = phi.r[i].value;
                    in.oper[1].type = Operand::LOCAL;
                    in.oper[1].value = var;
                    in.oper[1].tag = offset2tag[var];
                    prev->instr.push_back(in);
                }
            }

            if (prev->br_next == b) {
                prev->instr.push_back(bak);
                if (addr_updated)
                    for (Block* pp : prev->prevs)
                        pp->instr.back().set_br_addr(prev->instr[0].addr);
            }
        }
    }
}

void Block::ssa_rename_var(map<int, RenameStack>& stack)
{
    for (auto& var_phi : phi)
        var_phi.second.l = stack[var_phi.first].push();

    for (auto& in : instr) {
        if (in.oper[0].is_local())
            in.oper[0].ssa_idx = stack[in.oper[0].value].top();
        if (in.oper[1].is_local())
            in.oper[1].ssa_idx = stack[in.oper[1].value].top();

        if (in.op == Opcode::MOVE && in.oper[1].is_local())
            in.oper[1].ssa_idx = stack[in.oper[1].value].push();
    }

    rename_phi(seq_next, this, stack);
    rename_phi(br_next, this, stack);

    for (Block* c : domc)
        c->ssa_rename_var(stack);

    for (int var : defs)
        stack[var].pop();
}

static inline string var_name(const string& var, int idx)
{ 
    return var + std::to_string(idx);
}

static inline string var_name(const Operand& oper)
{
    if (oper.tag.empty() || oper.ssa_idx == -1) return string();
    return var_name(oper.tag, oper.ssa_idx);
}

static inline pair<string, int> operand(const Operand& oper)
{
    return make_pair(oper.tag, oper.ssa_idx);
}
*/

bool Phi::is_const() const
{
    for (const Operand& oper : r) {
        if (!oper.is_const()) return false;
        if (oper.value_const != r[0].value_const) return false;
    }
    return true;
}

bool Operand::operator< (const Operand& o) const
{
    if (type < o.type) return true;
    if (type > o.type) return false;
    if (_value < o._value) return true;
    if (_value > o._value) return false;
    return ssa_idx < o.ssa_idx;
}

void Function::ssa_constant_propagate()
{
    map<Operand, long long> const_val;

    bool updated = true;
    while (updated) {
        updated = false;

        for (Block* b : blocks) {
            for (auto& var_phi : b->phi) {
                Localvar* var = var_phi.first;
                Phi& phi = var_phi.second;
                assert(!phi.r.empty());

                for (Operand& oper : phi.r)
                    if (const_val.count(oper) > 0)
                        oper.to_const(const_val[oper]);

                if (phi.is_const()) {
                    const_val[ssa_oper(var, phi.l)] = phi.value();
                    phi.clear();
                    updated = true;
                }
            }

            for (Instruction* in : b->instr)
                if (in->op) {
                    for (int i = 0; i < 2; ++i)
                        if (in->isrightvalue(i) && const_val.count(in->oper[i]) > 0)
                                in->oper[i].to_const(const_val[in->oper[i]]);

                    if (in->isconst()) {
                        long long val = in->constvalue();
                        const_val[reg_oper(in)] = val;
                        if (in->is_move())
                            const_val[in->oper[1]] = val;
                        in->erase();
                        updated = true;
                    }
                }
        }
    }
}

void Phi::icode(FILE* out, Localvar* var) const
{
    if (r.empty()) return;
    fprintf(out, "instr %lld: phi", name);
    for (const Operand& oper : r) {
        if (oper.is_local()) {
            fprintf(out, " %s$%d", oper.var->name.c_str(), oper.ssa_idx);
        } else {
            assert(oper.is_const());
            fprintf(out, " %lld", oper.value_const);
        }
    }
    fprintf(out, "\ninstr %lld: move (%lld) %s$%d\n", name + 1, name, var->name.c_str(), l);
}

static inline map<Operand, Block*> calc_oper2block(Function* f)
{
    map<Operand, Block*> ret;
    for (Block* b : f->blocks)
        for (Instruction* in : b->instr) {
            if (in->is_move())
                ret[in->oper[1]] = b;
            ret[reg_oper(in)] = b;
        }
    return ret;
}

static inline vector<Block*> calc_loop_order(Function* f)
{
    unordered_map<Block*, int> cnt;
    for (auto& head_loop : f->loops)
        for (Block* b : head_loop.second)
            ++cnt[b];

    multimap<int, Block*> order;
    for (auto& block_cnt : cnt)
        order.insert(make_pair(block_cnt.second, block_cnt.first));

    vector<Block*> ret;
    for (auto& cnt_block : order)
        ret.push_back(cnt_block.second);
    return ret;
}

void Function::ssa_licm()
{
    map<Operand, Block*> oper2block = calc_oper2block(this);
    vector<Block*> loop_order = calc_loop_order(this);

    for (Block* b : loop_order) {
        for (Instruction* in : b->instr) {
            Block* b0 = nullptr;
            if (in->isrightvalue(0) && (in->oper[0].type == Operand::LOCAL || in->oper[0].type == Operand::REG))
                b0 = oper2block[in->oper[0]];
            Block* b1 = nullptr;
            if (in->isrightvalue(1) && (in->oper[1].type == Operand::LOCAL || in->oper[1].type == Operand::REG))
                b1 = oper2block[in->oper[1]];

            Block* last = nullptr;
            for (Block *loop_header = b; loop_header != NULL; loop_header = loop_header->idom) {
                for (; loop_header && loops.count(loop_header) == 0; loop_header = loop_header->idom) { }
                if (loop_header == nullptr) break;
                if (loops[loop_header].count(b0) > 0 || loops[loop_header].count(b1) > 0) break;
                last = loop_header;
            }

            if (last == nullptr) continue;

            last->insert.push_back(new Instruction(*in));
            in->erase();
        }
    }

    for (auto b_loop : loops) {
        Block* b = b_loop.first;
        if (b->insert.empty()) continue;

        Block* newb = new Block(this, b->insert.begin(), b->insert.end());

        vector<Block*> old_prevs;
        old_prevs.swap(b->prevs);

        newb->seq_next = b;
        newb->order_next = b;
        blocks.push_back(newb);

        for (Block* prev : old_prevs) {
            if (loops[b].count(prev) > 0) {
                b->prevs.push_back(prev);
                assert(prev->seq_next != b);
            } else {
                newb->prevs.push_back(prev);
                if (prev->seq_next == b) prev->seq_next = newb;
                if (prev->br_next == b) {
                    prev->br_next = newb;
                    prev->instr.back()->set_branch(newb);
                }
                if (prev->order_next == b) prev->order_next = newb;
            }
        }
    }
}

/*
void Program::ssa_icode(FILE* out)
{
    ssa_mode = true;
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

        for (Block* block = func->entry; block != nullptr; block = block->seq_next2) {
            long long phi_addr = block->ssa_addr;

            for (auto& var_phi : block->phi) {
                int var = var_phi.first;
                auto& phi = var_phi.second;
                if (phi.r.empty()) continue;
                string tag = func->offset2tag[var];

                fprintf(out, "instr %lld: phi", phi_addr);
                for (const Operand& oper : phi.r) {
                    if (oper.type == Operand::LOCAL) {
                        fprintf(out, " %s$%d", tag.c_str(), oper.ssa_idx);
                    } else {
                        assert(oper.type == Operand::CONST);
                        fprintf(out, " %lld", oper.value);
                    }
                }
                fputc('\n', out);

                fprintf(out, "instr %lld: move (%lld) %s$%d\n", phi_addr + 1, phi_addr, tag.c_str(), phi.l);

                phi_addr += 2;
            }

            for (int i = 0; i < block->instr.size() - 1; ++i)
                if (block->instr[i].op != Opcode::NOP)
                    block->instr[i].icode(out);

            Instruction& in = block->instr.back();
            if (in.op == Opcode::NOP) continue;
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
}

void Program::place_phi()
{
}

void Program::remove_phi()
{
    instr_cnt = instr.size();
    for (Function* func : funcs)
        func->remove_phi();
}

void Program::ssa_rename_var()
{
}
*/

void Program::ssa_licm()
{
    for (Function* func : funcs)
        func->ssa_licm();
}

void Program::ssa_prepare()
{
    for (Function* f : funcs)
        f->entry->compute_df();

    for (Function* f : funcs)
        for (Block* b : f->blocks)
            b->find_defs();

    for (Function* f : funcs)
        f->place_phi();

    for (Function* f : funcs) {
        map<Localvar*, RenameStack> stack;
        f->entry->ssa_rename_var(stack);
    }
}

void Program::ssa_constant_propagate()
{
    for (Function* f : funcs)
        f->ssa_constant_propagate();
}

void Block::append(Instruction* in)
{
    if (br_next == nullptr)
        instr.push_back(in);
    else {
        auto bak = instr.back();
        instr.back() = in;
        instr.push_back(bak);
    }
}

void Program::ssa_to_3addr()
{
    for (Function* f : funcs)
        for (Block* b : f->blocks)
            for (auto& var_phi : b->phi) {
                Localvar* var = var_phi.first;
                Phi& phi = var_phi.second;

                if (!phi.empty()) {
                    for (int i = 0; i < phi.r.size(); ++i) {
                        if (phi.r[i].is_const()) {
                            Instruction* in = new Instruction();
                            in->op.type = Opcode::MOVE;
                            in->oper[0].type = Operand::CONST;
                            in->oper[0].value_const = phi.r[i].value_const;
                            in->oper[1].type = Operand::LOCAL;
                            in->oper[1].var = var;
                            phi.pre[i]->append(in);
                        }
                    }
                    phi.clear();
                }
            }

    for (Function* f : funcs)
        for (Block* b : f->blocks)
            for (Instruction* in : b->instr) {
                in->oper[0].ssa_idx = -1;
                in->oper[1].ssa_idx = -1;
            }
}
