#pragma  once

#include "BasicBlock.hpp"
#include "Dominators.hpp"
#include "Function.hpp"
#include "PassManager.hpp"
#include "Module.hpp"
#include "Instruction.hpp"
#include "Value.hpp"
#include <memory>
#include <set>
#include <map>

class LocalCommonSubExpr :public Pass{
    public:
        LocalCommonSubExpr(Module *m) : Pass(m) {module_ = m;}
        void run() override;
    private:
        Module *module_;
        Function *func_;
        std::unique_ptr<Dominators> dominator;
        void find_common_subexpr(BasicBlock *bb);
        bool is_eliminatable_call(Function *func);
        void get_eliminatable_call();

        std::set<Function *> elim_call;
        std::map<BasicBlock *, std::map<std::tuple<Value *,Value *,Instruction::OpID>,Value *>> bin_cmp_expr;
        std::map<BasicBlock *, std::map<std::tuple<Value *,int,Instruction::OpID>,Value *>> bin_cmp_expr_riconst;
        std::map<BasicBlock *, std::map<std::tuple<int,Value *,Instruction::OpID>,Value *>> bin_cmp_expr_liconst;
        std::map<BasicBlock *, std::map<std::tuple<float,Value *,Instruction::OpID>,Value *>> bin_cmp_expr_lfconst;
        std::map<BasicBlock *, std::map<std::tuple<Value *,float,Instruction::OpID>,Value *>> bin_cmp_expr_rfconst;
        std::map<BasicBlock *, std::map<std::tuple<Value *,Value *>,Value *>> gep2_expr;
        std::map<BasicBlock *, std::map<std::tuple<Value *,int>,Value *>> gep2_expr_const;      
        std::map<BasicBlock *, std::map<std::tuple<Value *,Value *>,Value *>> gep3_expr;
        std::map<BasicBlock *, std::map<std::tuple<Value *,int>,Value *>> gep3_expr_const;
        std::map<BasicBlock *, std::map<Value *,Value *>> load_expr;
        std::map<BasicBlock *, std::map<Value *,std::tuple<Value *,Value*>>> load_gep_expr;
        std::map<BasicBlock *, std::map<Value *,std::tuple<int,Value*>>> load_gep_expr_const;
        std::map<BasicBlock *, std::map<std::pair<Function *,std::vector<Value *>>,Value *>> call_expr;

        std::set<BasicBlock *> visited;
};
