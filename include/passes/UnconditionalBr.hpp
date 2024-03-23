#include "BasicBlock.hpp"
#include "FuncInfo.hpp"
#include "Function.hpp"
#include "Instruction.hpp"
#include "PassManager.hpp"
#include "Module.hpp"


class UnconditionalBr : public Pass{
    public:
        UnconditionalBr(Module *m) : Pass(m) {module_ = m;}
        void run();
        void mark(Function * func);
        void sweep(Function *func);
        void remove_single_return(Function *func);
    private:
        Module *module_;
        void find_func();
        void delete_unused_func();
        std::list<Instruction *> br_list;
        std::set<Function *> func_list;
        std::list<BasicBlock *> reverse_post_order;
};