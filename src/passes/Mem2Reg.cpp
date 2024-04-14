#include "Mem2Reg.hpp"
#include "BasicBlock.hpp"
#include "GlobalVariable.hpp"
#include "IRBuilder.hpp"
#include "Instruction.hpp"
#include "Value.hpp"
#include "logging.hpp"

#include <cstddef>
#include <cstdio>
#include <memory>

std::map<Value *, std::vector<Value *>> val_stack;
    
void Mem2Reg::run() {
    // 创建支配树分析 Pass 的实例
    dominators_ = std::make_unique<Dominators>(m_);
    // 建立支配树
    //LOG_DEBUG << "dominators run";
    dominators_->run();
    // 以函数为单元遍历实现 Mem2Reg 算法
    for (auto &f : m_->get_functions()) {
       // phi2val.clear();
        if (f.is_declaration())
            continue;
        func_ = &f;
        if (func_->get_basic_blocks().size() >= 1) {
            // 对应伪代码中 phi 指令插入的阶段
          //  LOG_DEBUG << "generate_phi " << func_->get_name();
            generate_phi();
            // 对应伪代码中重命名阶段
          //  LOG_DEBUG << "rename " << func_->get_name();
            rename(func_->get_entry_block());
        }
        // 后续 DeadCode 将移除冗余的局部变量的分配空间
    }
    LOG_DEBUG << m_->print();
}

void Mem2Reg::generate_phi() {
    // TODO
    // 步骤一：找到活跃在多个 block 的全局名字集合，以及它们所属的 bb 块
    std::set<Value *> globals;
    std::map<Value *, std::set<BasicBlock *>> global2bb;
    for(auto &bb1: func_->get_basic_blocks()){
        auto bb = &bb1;
        for(auto &inst1: bb->get_instructions()){
            auto inst = &inst1;
            if(inst->is_store()){
                auto lval = static_cast<StoreInst *>(inst)->get_lval();
                if(is_valid_ptr(lval))
                {
                    globals.insert(lval);
                    global2bb[lval].insert(bb);
                }
            }
        }
    }

    // 步骤二：从支配树获取支配边界信息，并在对应位置插入 phi 指令
    std::map<std::pair<BasicBlock *,Value *>,bool> bb_has_var_phi;
    for(auto var : globals)
    {
        std::vector<BasicBlock *> worklist;
        worklist.assign(global2bb[var].begin(), global2bb[var].end());
       // //printf("worklist size: %d\n", worklist.size());
        for(int i = 0;i < worklist.size();i ++)   
        {
            auto bb = worklist[i];
            for(auto bb_dominance_frontier: dominators_->get_dominance_frontier(bb))
            {
                ////printf("bb_dominance_frontier: %s\n", bb_dominance_frontier->get_name().c_str());
                if(bb_has_var_phi.find({bb_dominance_frontier,var}) == bb_has_var_phi.end())
                {
                    //printf("phi: create\n");
                    auto phi = PhiInst::create_phi(var->get_type()->get_pointer_element_type(),bb_dominance_frontier);
                    ////printf("phi: %s\n", phi->get_name().c_str());
                    phi2val[phi] = var;
                    bb_dominance_frontier->add_instr_begin(phi);   
                    worklist.push_back(bb_dominance_frontier);
                    bb_has_var_phi[{bb_dominance_frontier,var}] = true;
                }
            }
        }
    }
}

int counter = 0;
void Mem2Reg::rename(BasicBlock *bb) {
    // TODO
    // 步骤三：将 phi 指令作为 lval 的最新定值，lval 即是为局部变量 alloca 出的地址空间
    ////printf("%s\n", bb->print().c_str());
    std::vector<Instruction *> to_remove;
   
    //printf("step 3: %s\n", bb->get_name().c_str());
    for(auto &inst1 : bb->get_instructions())
    {
        auto inst = &inst1;
        if(inst->is_phi())
        {
           // //printf("phi: find\n" );
            auto phi = static_cast<PhiInst *>(inst);
            auto lval = phi2val[phi];
            val_stack[lval].push_back(inst);
        }
    }
    // 步骤四：用 lval 最新的定值替代对应的load指令
    //printf("step 4: %s\n", bb->get_name().c_str());
    for(auto &inst1 : bb->get_instructions())
    {
        auto inst = &inst1;
        if(inst->is_load())
        {
            auto load = static_cast<LoadInst *>(inst);
            auto lval = load->get_lval();

            if(is_valid_ptr(lval))
            {
               // //printf("lval: %s\n", lval->get_name().c_str());
                if(val_stack.find(lval) != val_stack.end())
                {
                    //("load: replace\n");
                    auto phi = val_stack[lval].back();
                    inst->replace_all_use_with(phi);
                    //printf("load: %s\n", inst->print().c_str());
                    //to_remove.push_back(inst);
                }
            }
        }
    
    // 步骤五：将 store 指令的 rval，也即被存入内存的值，作为 lval 的最新定值
        //printf("step 5: %s\n", bb->get_name().c_str());
        if(inst->is_store())
        {
            auto store = static_cast<StoreInst *>(inst);
            auto lval = store->get_lval();
            auto rval = store->get_rval();
            if(is_valid_ptr(lval))
            {
                val_stack[lval].push_back(rval);
                //printf("store: %s\n", inst->print().c_str());
                to_remove.push_back(inst);
            }
        }
    }
    // 步骤六：为 lval 对应的 phi 指令参数补充完整
    //printf("step 6: %s %d size\n", bb->get_name().c_str(),bb->get_succ_basic_blocks().size());
     //printf("%s\n",bb->print().c_str());
    for(auto bb1 : bb->get_succ_basic_blocks())
    {
        ////printf("bb1: %s\n", bb1->get_name().c_str());
        ////printf("%s\n",bb1->print().c_str());
        for(auto &inst1 : bb1->get_instructions())
        {
            ////printf("inst1: %s\n", inst1.print().c_str());
            auto inst = &inst1;
            if(inst->is_phi())
            {
                //printf("phi: find\n");
                auto phi = static_cast<PhiInst *>(inst);
                auto lval = phi2val[phi];
                if(val_stack.find(lval) != val_stack.end() && !val_stack[lval].empty())
                {
                    auto rval = val_stack[lval].back();
                    phi->add_phi_pair_operand(rval,bb);
                }
            }
        }
    }
    //printf("step 7: %s  %d \n", bb->get_name().c_str(),dominators_->get_dom_tree_succ_blocks(bb).size());    
    // 步骤七：对 bb 在支配树上的所有后继节点，递归执行 re_name 操作
    for(auto &bb1: dominators_->get_dom_tree_succ_blocks(bb))
    {
        //printf("enter %s\n", bb1->get_name().c_str());
        rename(bb1);
    }
    // 步骤八：pop出 lval 的最新定值
    for(auto &inst1 : bb->get_instructions())
    {
        auto inst = &inst1;
        if(inst->is_phi())
        {
            auto phi = static_cast<PhiInst *>(inst);
            auto lval = phi2val[phi];
            if(val_stack.find(lval) != val_stack.end())
            {            
                val_stack[lval].pop_back();
            }       
       }
        else if(inst->is_store())
        {
            auto store = static_cast<StoreInst *>(inst);
            auto lval = store->get_lval();
            if(is_valid_ptr(lval))
            {
                if(val_stack.find(lval) != val_stack.end())
                {
                    val_stack[lval].pop_back();
                }
            }
        }
    }
    // 步骤九：清除冗余的指令
    //printf("step 9: %s\n", bb->get_name().c_str());
    for(auto inst : to_remove)
    {
        //printf("remove: %s\n", inst->print().c_str());
        bb->erase_instr(inst);
    }
   
    //printf("counter: %d\n", counter);
}

