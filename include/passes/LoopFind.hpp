#pragma once

#include "Function.hpp"
#include "Module.hpp"
#include "PassManager.hpp"
#include <memory>
#include <stack>

using Loop = std::vector<BasicBlock *>;
class LoopFind : public Pass {
    public:
        LoopFind(Module *m) : Pass(m) {module_ = m;}
        void run() override;
        void loop_find(Function *func);
        void tarjan(BasicBlock *bb);

        BasicBlock* get_loop_entry(Loop* loop){return *(*loop).rbegin();}
    
        //& Loopinvariant
        std::vector<Loop *>* get_loops() { return &loops; }
        Loop* get_bb_loop(BasicBlock* BB){return bb_loop[BB];}
        Loop* get_parent_loop(std::vector<BasicBlock*>* BB){return parent_loop[BB];}
    private:
        Module *module_;
        std::vector<Loop*> loops;
        std::map<BasicBlock *, Loop*> bb_loop;
        std::map<Loop*, Loop*> parent_loop;
        std::map<BasicBlock *, int> color;
        int cnt = 0;
        std::map<BasicBlock*,int> BB_DFN;
        std::map<BasicBlock*,int> BB_LOW;
        std::stack<BasicBlock*> BB_Stack;
      //  std::shared_ptr<Loop> loop;
        std::stack<Loop*> loop_stack;
};



