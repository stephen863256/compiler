#pragma once
#include "Function.hpp"
#include "Instruction.hpp"
#include "Module.hpp"
#include "PassManager.hpp"
#include "Value.hpp"
#include <vector>


class FunctionInline : public Pass {
    public:
        FunctionInline(Module *m) : Pass(m) {module_ = m;}
        void run() override;

    private:
        Module *module_;
        void inline_func_find();
        void inline_func();
        int MAX_INLINE = 10;
        std::set<std::pair<Instruction *,Function *>> call_list;
        //std::list <std::pair<Value *,BasicBlock *>> mul_ret_list;
        //std::map<Value *,Value *> arg_map;
        std::map<Value *,Value *> call_inst_map;
        std::list<Instruction *> new_inst_list;
        Value *ret_val = nullptr;
};