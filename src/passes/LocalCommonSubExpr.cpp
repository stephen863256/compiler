#include "BasicBlock.hpp"
#include "Dominators.hpp"
#include "Function.hpp"
#include "Instruction.hpp"
#include "LocalCommonSubExpr.hpp"
#include "Value.hpp"
#include "logging.hpp"
#include <stack>
void LocalCommonSubExpr::run() {
    dominator = std::make_unique<Dominators>(module_);
    dominator->run();
    get_eliminatable_call();
    LOG_DEBUG << "LocalCommonSubExpr start" ;
    //LOG_INFO << module_->print();
    //LOG_INFO << module_->get_functions().size() << " functions in module";
    for(auto &func : module_->get_functions()) {
        func_ = &func;
        if(func_->is_declaration()) continue;
      //  LOG_DEBUG << "LocalCommonSubExpr start in " << func.get_name();
        visited.clear();
        bin_cmp_expr.clear();
        bin_cmp_expr_riconst.clear();
        bin_cmp_expr_liconst.clear();
        bin_cmp_expr_rfconst.clear();
        bin_cmp_expr_lfconst.clear();
        gep2_expr.clear();
        gep2_expr_const.clear();
        gep3_expr.clear();
        gep3_expr_const.clear();
        load_expr.clear();
        //load_expr_const.clear();
        call_expr.clear();
        LOG_DEBUG << "LocalCommonSubExpr start in " << func.get_name() <<"\n" <<func.print();
        for(auto &bb : func.get_basic_blocks()) {
            if(visited.find(&bb) == visited.end()) {
                find_common_subexpr(&bb);
            }
        }
         LOG_DEBUG << "LocalCommonSubExpr end in " << func.get_name() <<"\n" <<func.print();
    }
}


void LocalCommonSubExpr::get_eliminatable_call() {
    for (auto &func : module_->get_functions()) {
        bool is_elim = true;
        if(func.get_basic_blocks().size() == 0) continue;
        for (auto &bb : func.get_basic_blocks()) {
            for (auto &inst : bb.get_instructions()) {
                if (inst.is_store()) {
                    is_elim = false;
                }
                if(inst.is_call()) {
                    auto call_func = dynamic_cast<Function *>(inst.get_operand(0));
                    if(elim_call.find(call_func) == elim_call.end() && call_func != &func) {
                        is_elim = false;
                    }
                }
            }
        }
        if(is_elim) {
            elim_call.insert(&func);
        }
    }
}

inline bool LocalCommonSubExpr::is_eliminatable_call(Function *func) {
    return elim_call.find(func) != elim_call.end();
}

void LocalCommonSubExpr::find_common_subexpr(BasicBlock *bb){
    LOG_DEBUG << "find common subexpr in " << bb->get_name();
    if(bb != func_->get_entry_block()){
        auto idom_bb = dominator->get_idom(bb);
        if(idom_bb != nullptr)
        {
        LOG_DEBUG << "idom is " << idom_bb->get_name();
        if(visited.find(idom_bb) == visited.end()) {
            find_common_subexpr(idom_bb);
        }

        bin_cmp_expr[bb] = bin_cmp_expr[idom_bb];
        bin_cmp_expr_riconst[bb] = bin_cmp_expr_riconst[idom_bb];
        bin_cmp_expr_liconst[bb] = bin_cmp_expr_liconst[idom_bb];
        bin_cmp_expr_rfconst[bb] = bin_cmp_expr_rfconst[idom_bb];
        bin_cmp_expr_lfconst[bb] = bin_cmp_expr_lfconst[idom_bb];
        gep2_expr[bb] = gep2_expr[idom_bb];
        gep2_expr_const[bb] = gep2_expr_const[idom_bb];
        gep3_expr[bb] = gep3_expr[idom_bb];
        gep3_expr_const[bb] = gep3_expr_const[idom_bb];
      //  load_expr[bb] = load_expr[idom_bb];
        }
    }
        for(auto &inst : bb->get_instructions()) {
            if(inst.is_int_binary())
            {
                auto lhs = inst.get_operand(0);
                auto rhs = inst.get_operand(1);
                auto lhs_const = dynamic_cast<ConstantInt *>(lhs);
                auto rhs_const = dynamic_cast<ConstantInt *>(rhs);
                if(rhs_const)
                {
                    auto iter = bin_cmp_expr_riconst[bb].find({lhs,rhs_const->get_value(),inst.get_instr_type()});
                    if(iter != bin_cmp_expr_riconst[bb].end())
                    {
                        inst.replace_all_use_with(iter->second);
                    }
                    else {
                        bin_cmp_expr_riconst[bb].insert({{lhs,rhs_const->get_value(),inst.get_instr_type()},&inst});
                    }
                }
                else if(lhs_const)
                {
                    auto iter = bin_cmp_expr_liconst[bb].find({lhs_const->get_value(),rhs,inst.get_instr_type()});
                    if(iter != bin_cmp_expr_liconst[bb].end())
                    {
                        inst.replace_all_use_with(iter->second);
                    }
                    else {
                        bin_cmp_expr_liconst[bb].insert({{lhs_const->get_value(),rhs,inst.get_instr_type()},&inst});
                    }
                }
                else
                {
                    auto iter = bin_cmp_expr[bb].find({lhs,rhs,inst.get_instr_type()});
                    if(iter != bin_cmp_expr[bb].end())
                    {
                        inst.replace_all_use_with(iter->second);
                    }
                    else {
                        bin_cmp_expr[bb].insert({{lhs,rhs,inst.get_instr_type()},&inst});
                    }
                }
            }
            else if(inst.is_float_binary())
            {
                auto lhs = inst.get_operand(0);
                auto rhs = inst.get_operand(1);
                auto lhs_const = dynamic_cast<ConstantFP *>(lhs);
                auto rhs_const = dynamic_cast<ConstantFP *>(rhs);
                if(rhs_const)
                {
                    auto iter = bin_cmp_expr_rfconst[bb].find({lhs,rhs_const->get_value(),inst.get_instr_type()});
                    if(iter != bin_cmp_expr_rfconst[bb].end())
                    {
                        inst.replace_all_use_with(iter->second);
                    }
                    else {
                        bin_cmp_expr_rfconst[bb].insert({{lhs,rhs_const->get_value(),inst.get_instr_type()},&inst});
                    }
                }
                else if(lhs_const)
                {
                    auto iter = bin_cmp_expr_lfconst[bb].find({lhs_const->get_value(),rhs,inst.get_instr_type()});
                    if(iter != bin_cmp_expr_lfconst[bb].end())
                    {
                        inst.replace_all_use_with(iter->second);
                    }
                    else {
                        bin_cmp_expr_lfconst[bb].insert({{lhs_const->get_value(),rhs,inst.get_instr_type()},&inst});
                    }
                }
                else
                {
                    auto iter = bin_cmp_expr[bb].find({lhs,rhs,inst.get_instr_type()});
                    if(iter != bin_cmp_expr[bb].end())
                    {
                        inst.replace_all_use_with(iter->second);
                    }
                    else {
                        bin_cmp_expr[bb].insert({{lhs,rhs,inst.get_instr_type()},&inst});
                    }
                }
            }
            else if(inst.is_gep())
            {
                auto ptr = inst.get_operand(0);
                if(inst.get_num_operand() == 2)
                {
                    auto idx = inst.get_operand(1);
                    auto idx_const = dynamic_cast<ConstantInt *>(idx);
                    if(idx_const)
                    {
                        auto iter = gep2_expr_const[bb].find({ptr,idx_const->get_value()});
                        if(iter != gep2_expr_const[bb].end())
                        {
                            LOG_DEBUG << "erase  gep in " << bb->get_name() << "\n\n" << inst.print() ;
                            inst.replace_all_use_with(iter->second);
                        }
                        else {
                             LOG_DEBUG << "const find gep in " << bb->get_name() << "\n\n" << inst.print() ;
                            gep2_expr_const[bb].insert({{ptr,idx_const->get_value()},&inst});
                        }
                    }
                    else
                    {
                        auto iter = gep2_expr[bb].find({ptr,idx});
                        if(iter != gep2_expr[bb].end())
                        {
                             LOG_DEBUG << "erase  gep in " << bb->get_name() << "\n\n" << inst.print() ;
                            inst.replace_all_use_with(iter->second);
                        }
                        else {
                             LOG_DEBUG << "find gep in " << bb->get_name() << "\n\n" << inst.print() ;
                            gep2_expr[bb].insert({{ptr,idx},&inst});
                        }
                    }
                }
                else if(inst.get_num_operand() == 3)
                {
                   
                    auto idx = inst.get_operand(2);
                    auto idx_const = dynamic_cast<ConstantInt *>(idx);
                    if(idx_const)
                    {
                        auto iter = gep3_expr_const[bb].find({ptr,idx_const->get_value()});
                        if(iter != gep3_expr_const[bb].end())
                        {
                             LOG_DEBUG << "erase  gep in " << bb->get_name() << "\n\n" << inst.print() ;
                            inst.replace_all_use_with(iter->second);
                        }
                        else {
                            LOG_DEBUG << " const find gep in " << bb->get_name() << "\n\n" << inst.print() ;
                            gep3_expr_const[bb].insert({{ptr,idx_const->get_value()},&inst});
                        }
                    }
                    else
                    {
                        auto iter = gep3_expr[bb].find({ptr,idx});
                        if(iter != gep3_expr[bb].end())
                        {
                             LOG_DEBUG << "erase  gep in " << bb->get_name() << "\n\n" << inst.print() ;
                            inst.replace_all_use_with(iter->second);
                        }
                        else {
                             LOG_DEBUG << "find gep in " << bb->get_name() << "\n\n" << inst.print() ;
                            gep3_expr[bb].insert({{ptr,idx},&inst});
                        }
                    }
                }
            }
            else if(inst.is_load())
            {
                auto ptr = dynamic_cast<LoadInst *>(&inst)->get_lval();
                auto gep = dynamic_cast<GetElementPtrInst *>(ptr);
                if(gep != nullptr)
                {
                    auto idx = gep->get_operand(1);
                    if(gep->get_num_operand() == 3)
                    {   
                        idx = gep->get_operand(2);
                    }
                    auto idx_const = dynamic_cast<ConstantInt *>(idx);
                    if(idx_const)
                    {
                        auto iter = load_gep_expr_const[bb].find({gep});
                        if(iter != load_gep_expr_const[bb].end() && std::get<0>(iter->second) == idx_const->get_value())
                        {
                            LOG_DEBUG << "erase  load const in " << bb->get_name() << "\n\n" << inst.print() ;
                            inst.replace_all_use_with(std::get<1>(iter->second));
                        }
                        else {
                            LOG_DEBUG << "const find load const in " << bb->get_name() << "\n\n" << inst.print() ;
                            load_gep_expr_const[bb].insert({gep->get_operand(0),{idx_const->get_value(),&inst}});
                        }
                    }
                    else
                    {
                        auto iter = load_gep_expr[bb].find(gep);
                        if(iter != load_gep_expr[bb].end() && std::get<0>(iter->second) == idx)
                        {
                            LOG_DEBUG << "erase  load gep in " << bb->get_name() << "\n\n" << inst.print() ;
                            inst.replace_all_use_with(std::get<1>(iter->second));
                        }
                        else {
                            LOG_DEBUG << "find load gep in " << bb->get_name() << "\n\n" << inst.print() ;
                            load_gep_expr[bb].insert({gep->get_operand(0),{idx,&inst}});
                        }
                    }
                }
                else
                {
                auto iter = load_expr[bb].find(ptr);
                if(iter != load_expr[bb].end())
                {
                    LOG_DEBUG << "erase  2load in " << bb->get_name() << "\n\n" << inst.print() ;
                    inst.replace_all_use_with(iter->second);
                }
                else 
                {
                    LOG_DEBUG << "find 2load in " << bb->get_name() << "\n\n" << inst.print() ;
                    load_expr[bb].insert({ptr,&inst});
                    LOG_DEBUG << "find 2load2 in " << bb->get_name() << "\n\n" << inst.print() ;
                }
            }
        }
        else if(inst.is_store())
        {
            auto ptr = dynamic_cast<StoreInst *>(&inst)->get_lval();
            auto gep = dynamic_cast<GetElementPtrInst *>(ptr);
            if(gep != nullptr)
            {
                auto idx = gep->get_operand(1);
                if(gep->get_num_operand() == 3)
                {   
                    idx = gep->get_operand(2);
                }
                auto idx_const = dynamic_cast<ConstantInt *>(idx);
                if(idx_const)
                {
                    auto iter = load_gep_expr_const[bb].find({gep->get_operand(0)});
                    if(iter != load_gep_expr_const[bb].end() && std::get<0>(iter->second) == idx_const->get_value())
                    {
                        LOG_DEBUG << "store erase  load const in " << bb->get_name() << "\n\n" << inst.print() ;
                        load_gep_expr_const[bb].erase(iter);
                    }
                    auto idom_bb = dominator->get_idom(bb);
                    auto iter2 = load_gep_expr_const[idom_bb].find({gep});
                    if(iter2 != load_gep_expr_const[idom_bb].end() && std::get<0>(iter2->second) == idx_const->get_value())
                    {
                        load_gep_expr_const[idom_bb].erase(iter2);
                    }
                }
                else
                {
                    auto iter = load_gep_expr[bb].find(gep->get_operand(0));
                    if(iter != load_gep_expr[bb].end() && std::get<0>(iter->second) == idx)
                    {
                        LOG_DEBUG << "store erase  load gep in " << bb->get_name() << "\n\n" << inst.print() ;
                        load_gep_expr[bb].erase(iter);
                    }
                    auto idom_bb = dominator->get_idom(bb);
                    auto iter2 = load_gep_expr[idom_bb].find(gep);
                    if(iter2 != load_gep_expr[idom_bb].end() && std::get<0>(iter2->second) == idx)
                    {
                        load_gep_expr[idom_bb].erase(iter2);
                    }
                }            
            }
          
            //auto val = dynamic_cast<StoreInst *>(&inst)->get_rval();
            auto iter = load_expr[bb].find(ptr);
            if(iter != load_expr[bb].end())
            {
                LOG_DEBUG << "store erase  load in " << bb->get_name() << "\n\n" << inst.print() <<"\n\n" << iter->second->print();
                load_expr[bb].erase(iter);  
            }
            auto idom_bb = dominator->get_idom(bb);
            auto iter2 = load_expr[idom_bb].find(ptr);
            if(iter2 != load_expr[idom_bb].end())
            {
                LOG_DEBUG << "store1111 erase  load in " << idom_bb->get_name() << "\n\n" << inst.print() ;
                load_expr[idom_bb].erase(iter2);
            }
        }
        else if(inst.is_call())
        {
            auto call_func = dynamic_cast<Function *>(inst.get_operand(0));
            std::vector<Value *> args;
            for(int i = 1; i < inst.get_num_operand(); i++)
            {
                args.push_back(inst.get_operand(i));
            }
            if(is_eliminatable_call(call_func))
            {
                auto iter = call_expr[bb].find({call_func,args});
                if(iter != call_expr[bb].end())
                {
                    inst.replace_all_use_with(iter->second);
                }
                else 
                {
                    call_expr[bb].insert({{call_func,args},&inst});
                }
                continue;
            }
            if(call_func->is_declaration()) continue;
            for(auto &arg : args)
            {
                LOG_DEBUG << "call find load in " <<call_func->get_name() << "\n\n" << inst.print() ;
                auto gep = dynamic_cast<GetElementPtrInst *>(arg);
                if(gep == nullptr) continue;
                LOG_DEBUG << arg->print();
                auto iter = load_gep_expr[bb].find(gep->get_operand(0));
                if(iter != load_gep_expr[bb].end())
                {
                    LOG_DEBUG << "erase  load in " << bb->get_name() << "\n\n" << inst.print() ;
                    load_gep_expr[bb].erase(iter);
                }
                auto idom_bb = dominator->get_idom(bb);
                auto iter2 = load_gep_expr[idom_bb].find(gep->get_operand(0));
                if(iter2 != load_gep_expr[idom_bb].end())
                {
                    load_gep_expr[idom_bb].erase(iter2);
                }
                auto iter3 = load_gep_expr_const[bb].find({gep->get_operand(0)});
                if(iter3 != load_gep_expr_const[bb].end())
                {
                    LOG_DEBUG << "erase  load const in " << bb->get_name() << "\n\n" << std::get<1>(iter3->second)->print() ;
                    load_gep_expr_const[bb].erase(iter3);
                }
                auto iter4 = load_gep_expr_const[idom_bb].find(gep->get_operand(0));
                if(iter4 != load_gep_expr_const[idom_bb].end())
                {
                    load_gep_expr_const[idom_bb].erase(iter4);
                }
            }
            std::stack<Function *> call_stack;
            call_stack.push(call_func);
            while(!call_stack.empty())
            {
                auto call_func = call_stack.top();
                //auto args = call_stack.top().second;
                std::map<Argument *,Value *> arg_map;
                call_stack.pop();
                for(auto &bb_ : call_func->get_basic_blocks())
                {
                    for(auto &inst: bb_.get_instructions())
                    {
                        if(inst.is_store())
                        {
                            LOG_DEBUG << call_func->get_name() << " has store" << "  \n" << inst.print();
                            auto ptr = dynamic_cast<StoreInst *>(&inst)->get_lval();
                            auto iter = load_expr[bb].find(ptr);
                            if(iter != load_expr[bb].end())
                            {
                                LOG_DEBUG << "erase  load     in " << bb->get_name() << "\n\n" << inst.print() ;
                                load_expr[bb].erase(iter);
                            }
                            auto idom_bb = dominator->get_idom(bb);
                            auto iter2 = load_expr[idom_bb].find(ptr);
                            if(iter2 != load_expr[idom_bb].end())
                            {
                                load_expr[idom_bb].erase(iter2);
                            }
                        }
                        else if(inst.is_call())
                        {
                            auto call_func_ = dynamic_cast<Function *>(inst.get_operand(0));
                            if(call_func != call_func_ ){
                                LOG_DEBUG << "call stack push " << call_func_->get_name();
                                call_stack.push(call_func_);
                            }
                        }
                    }
                }
            }
        }
    }
   // LOG_DEBUG << "finish find common subexpr in " << bb->get_name();
    visited.insert(bb);
}

