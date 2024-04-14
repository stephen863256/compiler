#pragma once

#include "Instruction.hpp"
#include "Value.hpp"

#include <list>
#include <llvm/ADT/ilist.h>
#include <llvm/ADT/ilist_node.h>
#include <set>
#include <string>

class Function;
class Instruction;
class Module;

class BasicBlock : public Value, public llvm::ilist_node<BasicBlock> {
  public:
    ~BasicBlock() = default;
    static BasicBlock *create(Module *m, const std::string &name,
                              Function *parent) {
        auto prefix = name.empty() ? "" : "label_";
        return new BasicBlock(m, prefix + name, parent);
    }

    /****************api about cfg****************/
    std::list<BasicBlock *> &get_pre_basic_blocks() { return pre_bbs_; }
    std::list<BasicBlock *> &get_succ_basic_blocks() { return succ_bbs_; }
   
    void add_pre_basic_block(BasicBlock *bb) { pre_bbs_.push_back(bb); }
    void add_succ_basic_block(BasicBlock *bb) { succ_bbs_.push_back(bb); }
    void remove_pre_basic_block(BasicBlock *bb) { pre_bbs_.remove(bb); }
    void remove_succ_basic_block(BasicBlock *bb) { succ_bbs_.remove(bb); }

    // If the Block is terminated by ret/br
    bool is_terminated() const;
    // Get terminator, only accept valid case use
    Instruction *get_terminator();

    /****************api about Instruction****************/
    void add_instruction(Instruction *instr);
    void insert_instruction(llvm::ilist<Instruction>::iterator instr_pos, Instruction *instr)
    {
        instr_list_.insert(instr_pos, instr);
    }
    void insert_instruction(Instruction *old_instr, Instruction *new_instr)
    {
      auto iter_old = instr_list_.end();
      auto iter_new = instr_list_.end();
      for(auto iter = instr_list_.begin(); iter != instr_list_.end(); iter++)
      {
        if(&*iter == old_instr)
        {
          iter_old = iter;
        }
      }
      for(auto iter = instr_list_.begin(); iter != instr_list_.end(); iter++)
      {
        if(&*iter == new_instr)
        {
          iter_new = iter;
        }
      }
      if (iter_old != instr_list_.end() && iter_new != instr_list_.end()) {
          instr_list_.splice(std::next(iter_old), instr_list_, iter_new);
      }
    }
    void replace_instruction(Instruction *old_instr, Instruction *new_instr);
    void add_instr_begin(Instruction *instr) { instr_list_.push_front(instr); }
    void erase_instr(Instruction *instr) { instr_list_.erase(instr); }

    llvm::ilist<Instruction> &get_instructions() { return instr_list_; }
    bool empty() const { return instr_list_.empty(); }
    int get_num_of_instr() const { return instr_list_.size(); }

    /****************api about accessing parent****************/
    Function *get_parent() { return parent_; }
    Module *get_module();
    void erase_from_parent();
    int get_loop_depth() { return loop_depth_; }
    void loop_depth_add(int i) { loop_depth_ = loop_depth_ + i; }
    void loop_depth_reset() { loop_depth_ = 0; }
    int get_indegree() { return indegree_; }
    void indegree_add(int i) { indegree_ = indegree_ + i; }
    void indegree_reset() { indegree_ = 0; }
    bool  indegree_is_zero() { return indegree_ == 0; }
    virtual std::string print() override;
    void set_live_in_int(std::set<Value *> live_in) { ilive_in = live_in;}
    void set_live_out_int(std::set<Value *> live_out) { ilive_out = live_out;}
    void set_live_in_float(std::set<Value *> live_in) { flive_in = live_in;}
    void set_live_out_float(std::set<Value *> live_out) { flive_out = live_out;}
    std::set<Value *> get_live_in_int() { return ilive_in;}
    std::set<Value *> get_live_out_int() { return ilive_out;}
    std::set<Value *> get_live_in_float() { return flive_in;}
    std::set<Value *> get_live_out_float() { return flive_out;}
    void set_def(std::set<Value*> def) { this->def = def;}
    void set_use(std::set<Value*> use) { this->use = use;}
    std::set<Value*> get_def() { return def;}
    std::set<Value*> get_use() { return use;}

  private:
    BasicBlock(const BasicBlock &) = delete;
    explicit BasicBlock(Module *m, const std::string &name, Function *parent);
    int loop_depth_;
    int indegree_;
    std::set<Value *> flive_in, flive_out;
    std::set<Value *> ilive_in, ilive_out;
    std::set<Value*> def;
    std::set<Value*> use;
    std::list<BasicBlock *> pre_bbs_;
    std::list<BasicBlock *> succ_bbs_;
    llvm::ilist<Instruction> instr_list_;
    Function *parent_;
};
