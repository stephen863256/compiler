#include "UnconditionalBr.hpp"
#include "BasicBlock.hpp"
#include "Function.hpp"
#include "Instruction.hpp"
#include "Value.hpp"
#include "logging.hpp"
#include "Dominators.hpp"
#include <algorithm>
#include <list>

void UnconditionalBr::run() {
    Dominators dom(module_);
    LOG_DEBUG << "UnconditionalBr::run()";
    find_func();
    delete_unused_func();
    LOG_INFO << module_->print();
    for(auto &F : module_->get_functions()){
        auto func = &F;
        if(func->get_basic_blocks().size() == 0){
            continue;
        }    
        br_list.clear();
        bool changed = false;
        auto func_bb_size = func->get_basic_blocks().size();
        dom.create_reverse_post_order(func);
        reverse_post_order = dom.get_reverse_post_order();
        do
        {
            br_list.clear();
            mark(func);
            for(auto &br : br_list){
                LOG_DEBUG << "erase br: " << br->print();
            }
            LOG_DEBUG << "erase br_list size: " << br_list.size();
            if(br_list.size() == 0){
                break;
            }
            sweep(func);
            if(func->get_basic_blocks().size() != func_bb_size){
                LOG_DEBUG << "func name: " << func->get_name();
                func_bb_size = func->get_basic_blocks().size();
                changed = true;
            }
            else{
                changed = false;
            }
        }while(changed);
    }
    for(auto &F:module_->get_functions())
    {
        auto func = &F;
        if(func->get_name() != "main" && func->get_basic_blocks().size() != 0)
        {
            remove_single_return(func);
        }
    }
    func_list.clear();
    find_func();
    delete_unused_func();
    LOG_INFO << module_->print();
}

void UnconditionalBr::find_func(){
    std::list<Function *> func_list1;
    for(auto &F : module_->get_functions()){
        auto func = &F;
   //     LOG_DEBUG << "func name: " << func->get_name();
        if(func->get_name() == "main"){
            func_list.insert(func);
            func_list1.push_back(func);
            break;
        }
    }
  //  LOG_DEBUG << "find_func()";
    while(!func_list1.empty()){
        auto func = func_list1.front();
       // LOG_DEBUG << "enter "<< func->get_name();
        func_list1.pop_front();
        for(auto &bb : func->get_basic_blocks()){
            for(auto &inst : bb.get_instructions()){
                if(inst.is_call())
                {
                  //  LOG_DEBUG << "call inst: " << inst.print();
                    auto func = dynamic_cast<Function *>(inst.get_operand(0));
                  //  auto it = std::find(func_list.begin(), func_list.end(), func);
                    if(func_list.find(func) == func_list.end()){
                       // LOG_DEBUG << "find func name: " << func->get_name();
                        func_list.insert(func);
                        func_list1.push_back(func);
                    }
                }
            }
        }
    }
}
void UnconditionalBr::delete_unused_func(){
    std::list<Function *> func_list1;
    for(auto &F:module_->get_functions())
    {
        auto func = &F;
        //LOG_DEBUG << "enter " << func->get_name();
        if(func_list.find(func) == func_list.end()){
            //LOG_DEBUG << "erase "<< func->get_name();
            func_list1.push_back(func);
        }
    }
    for(auto &func : func_list1){
        module_->get_functions().erase(*func);
    }
 
}

void UnconditionalBr::mark(Function *func){
    for(auto &bb : reverse_post_order)
    {
        if(bb->is_terminated())
        {
            auto ter = bb->get_terminator();
            if(ter->is_ret())
            {
                continue;
            }
            else
            {
                auto br = dynamic_cast<BranchInst *>(ter);
                if(!br->is_cond_br())
                {
                    //如果后继块只有一个前驱块，那么说明可以删除这个块
                    auto bb = dynamic_cast<BasicBlock *>(br->get_operand(0));
                    if(bb->get_pre_basic_blocks().size() == 1)
                    {
                        br_list.push_back(br);
                    }
                }
            }
        }
    }
}

void UnconditionalBr::sweep(Function *func){
    //for(auto &br : br_list){
        auto br = br_list.front();
        auto bb = dynamic_cast<BasicBlock *>(br->get_operand(0));
        auto pre_bb = bb->get_pre_basic_blocks();
        auto pre_bb1 = pre_bb.front();
        LOG_DEBUG << br->print();
        LOG_DEBUG << pre_bb1->get_name();
        LOG_DEBUG << "pre_bb size: " << pre_bb.size();
        assert(pre_bb.size() == 1 && br->get_parent() == pre_bb1);

        pre_bb1->erase_instr(br);
        //auto succ_bb = bb->get_succ_basic_blocks();
        std::list<Instruction *> inst_list;
        for(auto &inst : bb->get_instructions()){
            inst_list.push_back(&inst);
        }
        for(auto &inst : inst_list)
        {
            LOG_DEBUG << "move inst: " << inst->print();
            if(!inst->is_br())
            {
                
                inst->set_parent(pre_bb1);
                pre_bb1->add_instruction(inst);
            }
            else
            {
                auto br = dynamic_cast<BranchInst *>(inst);
                if(br->is_cond_br())
                {
                    auto true_bb = dynamic_cast<BasicBlock *>(br->get_operand(1));
                    auto false_bb = dynamic_cast<BasicBlock *>(br->get_operand(2));
                    BranchInst::create_cond_br(br->get_operand(0),true_bb,false_bb,pre_bb1);   
                }
                else
                {
                    LOG_DEBUG << "create br: " << br->print();
                    auto new_br = BranchInst::create_br(dynamic_cast<BasicBlock *>(br->get_operand(0)),pre_bb1);
                }
            }
        }
        for(auto &succ_bb: bb->get_succ_basic_blocks())
        {
            for(auto &inst: succ_bb->get_instructions())
            {
                if(inst.is_phi())
                {
                    auto phi = dynamic_cast<PhiInst *>(&inst);
                    for(auto i = 1;i < phi->get_num_operand();i += 2)
                    {
                        if(phi->get_operand(i) == bb)
                        {
                            phi->remove_operand(i);
                            Value *v = phi->get_operand(i-1);
                            phi->remove_operand(i - 1);
                            phi->add_phi_pair_operand(v,pre_bb1);
                        }
                    }
                }
            }
        }
        LOG_DEBUG << "erase bb: " << bb->get_name();
     //   LOG_DEBUG <<  bb->get_succ_basic_blocks().front()->get_name();
      //  LOG_DEBUG <<  bb->get_succ_basic_blocks().front()->get_pre_basic_blocks().front()->get_name();
      //  bb->get_succ_basic_blocks().front()->get_pre_basic_blocks().remove(bb);
       // LOG_DEBUG <<  bb->get_pre_basic_blocks().size();
       // bb->get_pre_basic_blocks().front()->get_succ_basic_blocks().remove(bb);
       // for(auto &inst : inst_list)
      //  {
     //       LOG_DEBUG << "erase inst: " << inst->print();
     //      bb->erase_instr(inst);
     //   }
        func->remove(bb);
        LOG_INFO << module_->print();
    }
//}

void UnconditionalBr::remove_single_return(Function *func)
{
    
    auto entry_bb = func->get_entry_block();
    LOG_INFO << func->print();
    if(entry_bb->get_num_of_instr() == 1)
    {
        auto &inst = entry_bb->get_instructions().front();
        if(inst.is_ret())
        {
            LOG_DEBUG << "remove single return begin";
            auto ret_inst = dynamic_cast<ReturnInst *>(&inst);
            if(ret_inst->is_void_ret())
            {
                for(auto &func_: m_->get_functions())
                {
                    for(auto &bb : func_.get_basic_blocks())
                    {
                        for(auto &inst : bb.get_instructions())
                        {
                            if(inst.is_call())
                            {
                                auto call_inst = dynamic_cast<CallInst *>(&inst);
                                if(call_inst->get_operand(0) == func)
                                {
                                    bb.erase_instr(&inst);
                                }
                            }
                        }
                    }
                }
            }
            else 
            {
                std::list <Instruction *> move_list;
                auto val = ret_inst->get_operand(0);
                for(auto &func_: m_->get_functions())
                {
                    for(auto &bb : func_.get_basic_blocks())
                    {
                        for(auto &inst : bb.get_instructions())
                        {
                            if(inst.is_call())
                            {
                                LOG_DEBUG << "remove single return call inst: " << inst.print();
                                auto call_inst = dynamic_cast<CallInst *>(&inst);
                                if(call_inst->get_operand(0) == func)
                                {
                                    LOG_DEBUG << "value: " << val->print();
                                    call_inst->replace_all_use_with(val);
                                    move_list.push_back(&inst);
                                }
                            }
                        }
                        //for(auto &inst : move_list)
                       // {
                       //     bb.get_instructions().remove(inst);
                       // }
                    }
                }
            }
        }
    }
}