#pragma once

#include "BasicBlock.hpp"
#include "Function.hpp"
#include "GlobalVariable.hpp"
#include "Module.hpp"
#include "PassManager.hpp"
#include "Constant.hpp"
#include "logging.hpp"
#include <set>

class ConstFolder {
  public:
    ConstFolder(Module *m) : module_(m) {}
    Constant *compute(Instruction *instr, Constant *value1, Constant *value2);
    Constant *compute(Instruction *instr, Constant *value1);
   
  private:
    Module *module_;
};

class ConstProp :public Pass{
    public:
        ConstProp(Module *m):Pass(m){module_ = m;}
        void run();
        void const_fold(BasicBlock *bb);
        void find_dead_block(Function *func,BasicBlock *bb);
        void clear_dead_block(Function *func);
        void remove_single_phi(Function *func);
        void find_unchanged_global_vars();
        std::set<BasicBlock *> get_dead_blocks() { return dead_blocks; }
        BasicBlock * set_dead_block(BasicBlock *bb) { dead_blocks.insert(bb); }
        void clear_dead_block() { dead_blocks.clear(); }
     //   void remove_single_return(Function *func);
    private:
        std::set<BasicBlock *> dead_blocks;
        std::set<Value *> unchanged_global_vars;
        Module *module_;
};