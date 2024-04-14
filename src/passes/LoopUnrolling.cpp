#include "LoopUnrolling.hpp"
#include "BasicBlock.hpp"
#include "Instruction.hpp"
#include "Value.hpp"
#include "logging.hpp"

void LoopUnrolling::run() {
    loop_find = std::make_unique<LoopFind>(module_);
    loop_find->run();
    //LOG_DEBUG << "LoopUnrolling run";
    for(auto &loop : *loop_find->get_loops())
    {
        //if(loop->size() < 3)
        if(loop->size() > 2)
            continue;
        auto count = is_unrollable(loop);
       // LOG_DEBUG << "count: " << count;
        if(count >= 20 && count <= 200)
            unroll_const_loop(count,loop);
        else if(count == -1)
            unroll_loop(loop);
    }
}

ICmpInst *cmp_inst = nullptr;

int LoopUnrolling::is_unrollable(Loop *loop)
{
    auto entry_bb = loop_find->get_loop_entry(loop);
    auto exit_bb = loop_find->get_loop_exit(loop);
    if(entry_bb->get_pre_basic_blocks().size() > 2 || exit_bb->get_pre_basic_blocks().size() != 1
    || exit_bb->get_succ_basic_blocks().size() != 1)
        return 0;
    
    for(auto iter = entry_bb->get_instructions().begin(); iter != entry_bb->get_instructions().end(); iter++)
    {
        if(iter->is_cmp())
        {
            cmp_inst = dynamic_cast<ICmpInst*>(&*iter);
           // LOG_DEBUG << "cmp_inst: " << cmp_inst->print();
            //if(cmp_inst->is_ne())
                break;
           // auto iter_next = std::next(iter);
            //auto inst_next = &*iter_next;
            //LOG_DEBUG << "inst_next: " << inst_next->print();
            //auto iter_next_next = std::next(iter_next);
            //auto inst_next_next = &*iter_next_next;
            //LOG_DEBUG << "inst_next_next: " << inst_next_next->print();
            //auto iter_next_next_next = std::next(iter_next_next);
            //auto inst_next_next_next = &*iter_next_next_next;
            //LOG_DEBUG << "inst_next_next_next: " << inst_next_next_next->print();
            //if(inst_next->is_zext() && inst_next_next->is_cmp() && inst_next_next_next->is_br())
                break;
        }
    }
    auto terminator = entry_bb->get_terminator();
    if(terminator->is_br())
    {
        auto br_inst = dynamic_cast<BranchInst*>(terminator);
        if(!br_inst->is_cond_br())
            return 0;
        //auto cmp_op = cmp_inst->get_operand(0);
        auto lval = cmp_inst->get_operand(0);
        auto rval = cmp_inst->get_operand(1);
        auto lval_const = dynamic_cast<ConstantInt*>(lval);
        auto rval_const = dynamic_cast<ConstantInt*>(rval);
        if(lval_const == nullptr && rval_const == nullptr)
            return -1;

        auto var_pos = lval_const == nullptr ? 1 : 2;
        auto iter_end_int = lval_const == nullptr ?  rval_const->get_value() : lval_const->get_value();
        auto init_val = lval_const == nullptr ? lval : rval;
       // LOG_DEBUG << "var_pos: " << var_pos << " iter_end_int: " << iter_end_int << " init_val: " << init_val->print();
        auto cur_inst = dynamic_cast<Instruction*>(init_val);
        if(!cur_inst->is_phi() || cur_inst->get_num_operand() != 4)
            return 0;
        auto val1 = cur_inst->get_operand(0);
        auto val1_bb = dynamic_cast<BasicBlock*>(cur_inst->get_operand(1));
        auto val2 = cur_inst->get_operand(2);
      //  LOG_DEBUG << "val1: " << val1->print() << " val2: " << val2->print() << " val1_bb: " << val1_bb->get_name();
        int init_int = 0;
        if(loop_find->get_bb_loop(val1_bb) != loop)
        {
            cur_inst = dynamic_cast<Instruction*>(val2);
            auto const_init = dynamic_cast<ConstantInt*>(val1);
            if(const_init == nullptr)
                return 0;
            init_int = const_init->get_value();
        }
        else
        {
            cur_inst = dynamic_cast<Instruction*>(val1);
            auto const_init = dynamic_cast<ConstantInt*>(val2);
            if(const_init == nullptr)
                return 0;
            init_int = const_init->get_value();
        }
       // LOG_DEBUG << "init_int: " << init_int;
        int inc_int = 0;
        while(cur_inst != init_val)
        {
            if(cur_inst->is_add())
            {
                auto lval = cur_inst->get_operand(0);
                auto rval = cur_inst->get_operand(1);
                auto lval_const = dynamic_cast<ConstantInt*>(lval);
                auto rval_const = dynamic_cast<ConstantInt*>(rval);
                if(lval_const == nullptr && rval_const == nullptr)
                    return 0;
                auto var_pos = lval_const == nullptr ? 0 : 1;
                auto const_val = lval_const == nullptr ? rval_const->get_value() : lval_const->get_value();
                inc_int += const_val;
                cur_inst = dynamic_cast<Instruction*>(cur_inst->get_operand(var_pos));
            }
            else if(cur_inst->is_sub())
            {
                auto lval = cur_inst->get_operand(0);
                auto rval = cur_inst->get_operand(1);
                auto lval_const = dynamic_cast<ConstantInt*>(lval);
                auto rval_const = dynamic_cast<ConstantInt*>(rval);
                if(lval_const == nullptr && rval_const == nullptr)
                    return 0;
                auto var_pos = lval_const == nullptr ? 0 : 1;
                auto const_val = lval_const == nullptr ? rval_const->get_value() : lval_const->get_value();
                inc_int -= const_val;
                cur_inst = dynamic_cast<Instruction*>(cur_inst->get_operand(var_pos));
            }
            else
                return 0;
        }
        int gap = init_int - iter_end_int;
        int count = 0;
        if(cmp_inst->is_ne())
        {
            if(gap == 0)
                return 0;
            else
            {
                if(inc_int == 0)
                    return 0;
                while(gap != 0)
                {
                    gap += inc_int;
                    count++;
                }
            }
            return count;
        }
        else if(var_pos == 1)//  i > 10
        {
            if(cmp_inst->is_gt())
            {
                if(gap <= 0)
                    return 0;
                else
                {
                    if(inc_int >= 0)
                        return 0;
                    while(gap > 0)
                    {
                        gap += inc_int;
                        count++;
                    }
                }
            }
            else if(cmp_inst->is_ge())
            {
                if(gap < 0)
                    return 0;
                else
                {
                    if(inc_int >= 0)
                        return 0;
                    while(gap >= 0)
                    {
                        gap += inc_int;
                        count++;
                    }
                }
            }
            else if(cmp_inst->is_lt())
            {
                if(gap >= 0)
                    return 0;
                else
                {
                    if(inc_int <= 0)
                        return 0;
                    while(gap < 0)
                    {
                        gap += inc_int;
                        count++;
                    }
                }
            }
            else if(cmp_inst->is_le())
            {
                if(gap > 0)
                    return 0;
                else
                {
                    if(inc_int <= 0)
                        return 0;
                    while(gap <= 0)
                    {
                        gap += inc_int;
                        count++;
                    }
                }
            }
            return count;
        }
        else if(var_pos == 2)
        {
            if(cmp_inst->is_gt())
            {
                if(gap >= 0)
                    return 0;
                else
                {
                    if(inc_int <= 0)
                        return 0;
                    while(gap < 0)
                    {
                        gap += inc_int;
                        count++;
                    }
                }
            }
            else if(cmp_inst->is_ge())
            {
                if(gap > 0)
                    return 0;
                else
                {
                    if(inc_int <= 0)
                        return 0;
                    while(gap <= 0)
                    {
                        gap += inc_int;
                        count++;
                    }
                }
            }
            else if(cmp_inst->is_lt())
            {
                if(gap <= 0)
                    return 0;
                else
                {
                    if(inc_int >= 0)
                        return 0;
                    while(gap > 0)
                    {
                        gap += inc_int;
                        count++;
                    }
                }
            }
            else if(cmp_inst->is_le())
            {
                if(gap < 0)
                    return 0;
                else
                {
                    if(inc_int >= 0)
                        return 0;
                    while(gap >= 0)
                    {
                        gap += inc_int;
                        count++;
                    }
                }
            }
            return count;
        }
        else 
            return 0;
    }
    return 0;
}


void LoopUnrolling::unroll_const_loop(int count,Loop *loop)
{
    auto entry_bb = loop_find->get_loop_entry(loop);
    auto exit_bb = loop_find->get_loop_exit(loop);
    
    std::map<Value*,Value*> phi_from_outer;
    std::map<Value*,Value*> phi_from_inner;

    for(auto &inst: entry_bb->get_instructions())
    {
        if(inst.is_phi())
        { 
            auto phi_inst = dynamic_cast<PhiInst*>(&inst);
            auto val1 = phi_inst->get_operand(0);
            auto val2 = phi_inst->get_operand(2);
            auto val1_bb = dynamic_cast<BasicBlock*>(phi_inst->get_operand(1));
            if(loop_find->get_bb_loop(val1_bb) != loop)
            {
                phi_from_outer[&inst] = val1;
                phi_from_inner[&inst] = val2;
            }
            else
            {
                phi_from_outer[&inst] = val2;
                phi_from_inner[&inst] = val1;
            }
        }
    }
    std::map<Value*,Value*> phi_to_unroll_new;
    std::map<Value*,Value*> loop_to_unroll_new;

    auto entry_term = entry_bb->get_terminator();
    auto after_bb = dynamic_cast<BasicBlock*>(entry_term->get_operand(2));
    //LOG_DEBUG << "after_bb: " << after_bb->get_name();
    //LOG_DEBUG << entry_bb->get_instructions().size();
    entry_bb->get_instructions().pop_back();
    //LOG_DEBUG << entry_bb->get_instructions().size();
    auto current_inst = &(*std::prev(entry_bb->get_instructions().end()));
   // LOG_DEBUG << "current_inst: " << current_inst->print();
    auto exit_term = exit_bb->get_terminator();
    exit_bb->get_instructions().remove(exit_term);
    for(auto i = 0; i < count; i++)
    {
        for(auto &inst: exit_bb->get_instructions())
        {
           // LOG_DEBUG << "inst: " << inst.print();
            if(inst.is_add())
            {
                auto new_add = IBinaryInst::create_add(inst.get_operand(0),inst.get_operand(1),entry_bb);
                entry_bb->insert_instruction(current_inst,new_add);
                current_inst = new_add;
            }
            else if(inst.is_sub())
            {
                auto new_sub = IBinaryInst::create_sub(inst.get_operand(0),inst.get_operand(1),entry_bb);
                entry_bb->insert_instruction(current_inst,new_sub);
                current_inst = new_sub;
            }
            else if(inst.is_mul())
            {
                auto new_mul = IBinaryInst::create_mul(inst.get_operand(0),inst.get_operand(1),entry_bb);
                entry_bb->insert_instruction(current_inst,new_mul);
                current_inst = new_mul;
            }
            else if(inst.is_div())
            {
                auto new_div = IBinaryInst::create_sdiv(inst.get_operand(0),inst.get_operand(1),entry_bb);
                entry_bb->insert_instruction(current_inst,new_div);
                current_inst = new_div;
            }
            else if(inst.is_rem())
            {
                auto new_rem = IBinaryInst::create_srem(inst.get_operand(0),inst.get_operand(1),entry_bb);
                entry_bb->insert_instruction(current_inst,new_rem);
                current_inst = new_rem;
            }
            else if(inst.is_fadd())
            {
                auto new_fadd = FBinaryInst::create_fadd(inst.get_operand(0),inst.get_operand(1),entry_bb);
                entry_bb->insert_instruction(current_inst,new_fadd);
                current_inst = new_fadd;
            }
            else if(inst.is_fdiv())
            {
                auto new_fdiv = FBinaryInst::create_fdiv(inst.get_operand(0),inst.get_operand(1),entry_bb);
                entry_bb->insert_instruction(current_inst,new_fdiv);
                current_inst = new_fdiv;
            }
            else if(inst.is_fmul())
            {
                auto new_fmul = FBinaryInst::create_fmul(inst.get_operand(0),inst.get_operand(1),entry_bb);
                entry_bb->insert_instruction(current_inst,new_fmul);
                current_inst = new_fmul;
            }
            else if(inst.is_fsub())
            {
                auto new_fsub = FBinaryInst::create_fsub(inst.get_operand(0),inst.get_operand(1),entry_bb);
                entry_bb->insert_instruction(current_inst,new_fsub);
                current_inst = new_fsub;
            }
            else if(inst.is_cmp())
            {
                if(inst.get_instr_op_name() == "sge")
                {
                    auto new_sge = ICmpInst::create_ge(inst.get_operand(0),inst.get_operand(1),entry_bb);
                    entry_bb->insert_instruction(current_inst,new_sge);
                    current_inst = new_sge;
                }
                else if(inst.get_instr_op_name() == "sle")
                {
                    auto new_sle = ICmpInst::create_le(inst.get_operand(0),inst.get_operand(1),entry_bb);
                    entry_bb->insert_instruction(current_inst,new_sle);
                    current_inst = new_sle;
                }
                else if(inst.get_instr_op_name() == "sgt")
                {
                    auto new_sgt = ICmpInst::create_gt(inst.get_operand(0),inst.get_operand(1),entry_bb);
                    entry_bb->insert_instruction(current_inst,new_sgt);
                    current_inst = new_sgt;
                }
                else if(inst.get_instr_op_name() == "slt")
                {
                    auto new_slt = ICmpInst::create_lt(inst.get_operand(0),inst.get_operand(1),entry_bb);
                    entry_bb->insert_instruction(current_inst,new_slt);
                    current_inst = new_slt;
                }
                else if(inst.get_instr_op_name() == "eq")
                {
                    auto new_eq = ICmpInst::create_eq(inst.get_operand(0),inst.get_operand(1),entry_bb);
                    entry_bb->insert_instruction(current_inst,new_eq);
                    current_inst = new_eq;
                }
                else if(inst.get_instr_op_name() == "ne")
                {
                    auto new_ne = ICmpInst::create_ne(inst.get_operand(0),inst.get_operand(1),entry_bb);
                    entry_bb->insert_instruction(current_inst,new_ne);
                    current_inst = new_ne;
                }
            }
            else if(inst.is_fcmp())
            {
                if(inst.get_instr_op_name() == "fge")
                {
                    auto new_fge = FCmpInst::create_fge(inst.get_operand(0),inst.get_operand(1),entry_bb);
                    entry_bb->insert_instruction(current_inst,new_fge);
                    current_inst = new_fge;
                }
                else if(inst.get_instr_op_name() == "fle")
                {
                    auto new_fle = FCmpInst::create_fle(inst.get_operand(0),inst.get_operand(1),entry_bb);
                    entry_bb->insert_instruction(current_inst,new_fle);
                    current_inst = new_fle;
                }
                else if(inst.get_instr_op_name() == "fgt")
                {
                    auto new_fgt = FCmpInst::create_fgt(inst.get_operand(0),inst.get_operand(1),entry_bb);
                    entry_bb->insert_instruction(current_inst,new_fgt);
                    current_inst = new_fgt;
                }
                else if(inst.get_instr_op_name() == "flt")
                {
                    auto new_flt = FCmpInst::create_flt(inst.get_operand(0),inst.get_operand(1),entry_bb);
                    entry_bb->insert_instruction(current_inst,new_flt);
                    current_inst = new_flt;
                }
                else if(inst.get_instr_op_name() == "feq")
                {
                    auto new_feq = FCmpInst::create_feq(inst.get_operand(0),inst.get_operand(1),entry_bb);
                    entry_bb->insert_instruction(current_inst,new_feq);
                    current_inst = new_feq;
                }
                else if(inst.get_instr_op_name() == "fne")
                {
                    auto new_fne = FCmpInst::create_fne(inst.get_operand(0),inst.get_operand(1),entry_bb);
                    entry_bb->insert_instruction(current_inst,new_fne);
                    current_inst = new_fne;
                }
            }
            else if(inst.is_call())
            {
                std::vector<Value *> args;
                for(auto i = 1; i < inst.get_num_operand(); i++)
                {
                    args.push_back(inst.get_operand(i));
                }
                auto func = dynamic_cast<Function *>(inst.get_operand(0));
                auto new_call = CallInst::create_call(func,args,entry_bb);
                entry_bb->insert_instruction(current_inst,new_call);
                current_inst = new_call;
            }
            else if(inst.is_br())
            {
                auto br = dynamic_cast<BranchInst *>(&inst);
                if(br->is_cond_br())
                {
                    auto true_bb = dynamic_cast<BasicBlock *>(inst.get_operand(1));
                    auto false_bb = dynamic_cast<BasicBlock *>(inst.get_operand(2));
                    auto new_br = BranchInst::create_cond_br(inst.get_operand(0),true_bb,false_bb,entry_bb);
                    entry_bb->insert_instruction(current_inst,new_br);
                    current_inst = new_br;
                }
                else
                {
                    auto true_bb = dynamic_cast<BasicBlock *>(inst.get_operand(0));
                    auto new_br = BranchInst::create_br(true_bb,entry_bb);
                    entry_bb->insert_instruction(current_inst,new_br);
                    current_inst = new_br;
                }
            }
            else if(inst.is_ret())
            {
                
            }
            else if(inst.is_gep())
            {
                std::vector<Value *> idx;
                for(auto i = 1; i < inst.get_num_operand(); i++)
                {
                    idx.push_back(inst.get_operand(i));
                }
                auto new_gep = GetElementPtrInst::create_gep(inst.get_operand(0),idx,entry_bb);
                entry_bb->insert_instruction(current_inst,new_gep);
                current_inst = new_gep;
            }
            else if(inst.is_store())
            {
                auto store = dynamic_cast<StoreInst *>(&inst);
                auto lval = store->get_lval();
                auto rval = store->get_rval();
                auto new_store = StoreInst::create_store(rval,lval,entry_bb);
                entry_bb->insert_instruction(current_inst,new_store);
                current_inst = new_store;
            }
            else if(inst.is_load())
            {
                auto load = dynamic_cast<LoadInst *>(&inst);
                auto lval = load->get_lval();
                auto new_load = LoadInst::create_load(lval,entry_bb);
                entry_bb->insert_instruction(current_inst,new_load);
                current_inst = new_load;
            }
            else if(inst.is_alloca())
            {
                auto alloca = dynamic_cast<AllocaInst *>(&inst);
                auto new_alloca = AllocaInst::create_alloca(alloca->get_type(),entry_bb);
                entry_bb->insert_instruction(current_inst,new_alloca);
                current_inst = new_alloca;
            }
            else if(inst.is_zext())
            {
                auto zext = dynamic_cast<ZextInst *>(&inst);
                auto new_zext = ZextInst::create_zext(zext->get_operand(0),zext->get_dest_type(),entry_bb);
                entry_bb->insert_instruction(current_inst,new_zext);
                current_inst = new_zext;
            }
            else if(inst.is_fp2si())
            {
                auto fp2si = dynamic_cast<FpToSiInst *>(&inst);
                auto new_fp2si = FpToSiInst::create_fptosi(fp2si->get_operand(0),fp2si->get_dest_type(),entry_bb);
                entry_bb->insert_instruction(current_inst,new_fp2si);
            }
            else if(inst.is_si2fp())
            {
                auto si2fp = dynamic_cast<SiToFpInst *>(&inst);
                auto new_si2fp = SiToFpInst::create_sitofp(si2fp->get_operand(0),entry_bb);
                entry_bb->insert_instruction(current_inst,new_si2fp);
                current_inst = new_si2fp;
            }
            else if(inst.is_phi())
            {
                auto phi = dynamic_cast<PhiInst *>(&inst);
                std::vector<Value *> vals;
                std::vector<BasicBlock *> val_bbs;
                for(auto i = 0; i < phi->get_num_operand(); i+=2)
                {
                    vals.push_back(phi->get_operand(i));
                    val_bbs.push_back(dynamic_cast<BasicBlock *>(phi->get_operand(i+1)));
                }
                auto new_phi = PhiInst::create_phi(phi->get_type(),exit_bb,vals,val_bbs);
                entry_bb->insert_instruction(current_inst,new_phi);
                current_inst = new_phi;
            }
            for(auto idx = 0;idx < current_inst->get_num_operand();idx++)
            {
                auto operand = current_inst->get_operand(idx);
                if(i == 0){
                    if(phi_from_outer.find(operand) != phi_from_outer.end())
                        current_inst->set_operand(idx,phi_from_outer[operand]);
                }
                if(phi_to_unroll_new.find(operand) != phi_to_unroll_new.end())
                    current_inst->set_operand(idx,phi_to_unroll_new[operand]);
                else if(loop_to_unroll_new.find(operand) != loop_to_unroll_new.end())
                    current_inst->set_operand(idx,loop_to_unroll_new[operand]);
            }
            loop_to_unroll_new[&inst] = current_inst;
           // LOG_DEBUG << "current_inst: " << current_inst->print();
        }
         for(auto phi:phi_from_inner)
        {
           // LOG_DEBUG << "phi: " << phi.first->print() << " phi2:" << phi.second->print();
            phi_to_unroll_new[phi.first] = loop_to_unroll_new[phi.second];
        }
    }
   // LOG_DEBUG << "remove insts";
    std::list<Instruction *> to_remove1;
    std::list<Instruction *> to_remove2;
    std::list<Use> set_operand_insts;
    for(auto &inst:entry_bb->get_instructions())
    {
        if(inst.is_phi())
        {
            auto inner_val = phi_from_inner[&inst];
           // LOG_DEBUG << "inner_val: " << inner_val->print();
            auto inner_to_loop_val = loop_to_unroll_new[inner_val];
          //  LOG_DEBUG << "inner_to_loop_val: " << inner_to_loop_val->print();
           // LOG_DEBUG << inst.get_use_list().size();
            inst.replace_all_use_with(inner_to_loop_val);
          //  LOG_DEBUG << "done  ";
          //  LOG_DEBUG << "phi: " << inst.print();
            to_remove1.push_back(&inst);
        }
        else if(inst.is_cmp())
        {
           // LOG_DEBUG << "cmp_inst: " << inst.print();
            to_remove1.push_back(&inst);
            //auto iter = std::find(entry_bb->get_instructions().begin(),entry_bb->get_instructions().end(),inst);
            auto iter = entry_bb->get_instructions().begin();
            while(&*iter != &inst)
                iter++;
            auto iter_after = std::next(iter);
            auto inst_after = &(*iter_after);
            if(inst_after->is_zext())
                to_remove1.push_back(inst_after);
        }
    }
    //for(auto &use:set_operand_insts)
    //{
     //   auto user = dynamic_cast<Instruction*>(use.val_);
      //  user->set_operand(use.arg_no_,loop_to_unroll_new[phi_from_inner[user]]);
    //}

    BranchInst::create_br(after_bb,entry_bb);
    for(auto &inst:exit_bb->get_instructions())
        to_remove2.push_back(&inst);
    for(auto &inst:to_remove1)
    {
       // LOG_DEBUG << "remove inst: " << inst->print();
        entry_bb->get_instructions().remove(inst);
    }
    for(auto &inst:to_remove2)
        exit_bb->get_instructions().remove(inst);
    
    exit_bb->get_parent()->remove(exit_bb);

    //LOG_INFO << entry_bb->get_parent()->print();
   // LOG_INFO << entry_bb->print();
   // LOG_INFO << entry_bb->get_succ_basic_blocks().front()->get_name();
}

void LoopUnrolling::unroll_loop(Loop *loop)
{

}