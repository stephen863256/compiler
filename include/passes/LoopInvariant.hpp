#pragma once

#include "Instruction.hpp"
#include "LoopFind.hpp"
#include "Module.hpp"
#include "PassManager.hpp"
#include "Value.hpp"
#include <utility>
#include <vector>


class LoopInvariant : public Pass {
    public:
        LoopInvariant(Module *m) : Pass(m) {module_ = m;}
        void run() override;

        void find_loop_invariant(Loop *loop);
        void move_invariant_out(Loop *loop);
        bool after_call_is_invariant(Function *func,std::vector<Value*>args,LoadInst * inst);
    private:
        Module *module_;
        std::unique_ptr<LoopFind> loop_find;
        std::vector<std::pair<BasicBlock *, std::vector<Instruction *>>> loop_invariant;
};