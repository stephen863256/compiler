#include "RegAlloc.hpp"
#include "Constant.hpp"
#include "GlobalVariable.hpp"
#include "Instruction.hpp"
#include "logging.hpp"
#include <math.h>
void Interval::add_range(int from, int to){
    if(range_list.empty())
    {
        range_list.push_front(new Range(from, to));
        return;
    }
    auto top_range = *range_list.begin();
    if(from >= top_range->from && from <= top_range->to)
    {
        if( to > top_range->to) 
            top_range->to = to;
    }
    else if(from < top_range->from)
    {
        if(to <= top_range->to && to >= top_range->from)
        {
            top_range->from = from;
        }
        else
        {
            auto new_range = new Range(from,to);
            range_list.push_front(new_range);
        }
    }
    else
    {
        auto new_range = new Range(from,to);
        range_list.push_front(new_range);
    }
}
bool Interval::intersects(Interval *other)
{
    auto iter1 = range_list.begin();
    auto iter2 = other->range_list.begin();
    while(iter1 != range_list.end() && iter2 != other->range_list.end())
    {
        auto range1 = *iter1;
        auto range2 = *iter2;
        if(range1->to <= range2->from)
        {
            iter1++;
        }
        else if(range2->to <= range1->from)
        {
            iter2++;
        }
        else
        {
            return true;
        }
    }
    return false;
}

void RegAllocDriver::compute_reg_alloc()
{
    LOG_DEBUG << "Start RegAlloc";
    auto active_var = new ActiveVar(m);
    active_var->run();
    for(auto &func:m->get_functions())
    {
        if(func.is_declaration())
        {
            continue;
        }
        auto reg_alloc = new RegAlloc(&func);
        reg_alloc->run();
        ireg_alloc[&func] = reg_alloc->get_ireg_alloc();
        for(auto &interval:reg_alloc->get_ireg_alloc())
        {
            LOG_DEBUG << "ireg_alloc: " << interval.first->get_name() << " " << interval.second->reg_id;
        }
        freg_alloc[&func] = reg_alloc->get_freg_alloc();
        for(auto &interval:reg_alloc->get_freg_alloc())
        {
            LOG_DEBUG << "freg_alloc: " << interval.first->get_name() << " " << interval.second->reg_id;
        }
        block_order[&func] = reg_alloc->get_block_order();
    }
}

void RegAlloc::run()
{
    for(auto &ireg:remained_all_ireg_ids)
    {
        ireg2ActInter[ireg] = std::set<Interval*>();
    }
    for(auto &freg:remained_all_freg_ids)
    {
        freg2ActInter[freg] = std::set<Interval*>();
    }
    compute_block_order();
    number_operations();
    compute_bonus_and_cost();
    build_intervals();
    walk_intervals();
    for(auto &interval:interval_list)
    {
        LOG_DEBUG << "Interval: " << interval->value->get_name() << " " << interval->reg_id;
        for(auto &range:interval->range_list)
        {
            LOG_DEBUG << "Range: " << range->from << " " << range->to;
        }
        for(auto &pos:interval->use_position_list)
        {
            LOG_DEBUG << "Use Pos: " << pos;
        }
    }
}
void RegAlloc::walk_intervals()
{
    active.clear();
    ireg2ActInter.clear();
    freg2ActInter.clear();
    for(auto &interval:interval_list)
    {
        current = interval;
        int position = interval->range_list.front()->from;
        std::vector<Interval*> to_remove = {};
        for(auto &act_interval:active)
        {
            if((*(act_interval->range_list).rbegin())->to < position)
            {
                add_reg_to_pool(act_interval);
                to_remove.push_back(act_interval);
            }
        }
        for(auto &interval:to_remove)
        {
            active.erase(interval);
            if(interval->reg_id < 0)
            {
                continue;
            }
            LOG_DEBUG << "interval->reg_id    " << interval->reg_id << " " << interval->value->get_name();
            if(interval->is_float_type())
            {
                freg2ActInter[interval->reg_id].erase(interval);
            }
            else
            {
                ireg2ActInter[interval->reg_id].erase(interval);
            }
        }

        alloc_reg();
    }  
}
void RegAlloc::alloc_reg()
{
    if(current->is_float_type())
    {
        if(!remained_all_freg_ids.empty())
        {
            double bonus = -1;
            int current_reg_id = *remained_all_freg_ids.begin();
            for(auto &reg_id:remained_all_freg_ids)
            {
                double temp_bonus = 0;
                auto caller_it = caller_arg_bonus[current->value].find(reg_id);
                if (caller_it != caller_arg_bonus[current->value].end()) {
                    temp_bonus += caller_it->second;
                }

                auto callee_it = callee_arg_bonus[current->value].find(reg_id);
                if (callee_it != callee_arg_bonus[current->value].end()) {
                    temp_bonus += callee_it->second;
                }

                for(auto &phi:phi_bonus[current->value])
                {
                    if(fval2Inter.find(phi.first) != fval2Inter.end())
                    {
                        if(!fval2Inter[phi.first]->intersects(current))
                        {
                            if(fval2Inter[phi.first]->reg_id == reg_id)
                            {
                                temp_bonus += phi.second;
                            }
                        }
                    }
                }
                if(reg_id == 0){
                    temp_bonus += ret_bonus[current->value];
                    temp_bonus += call_bonus[current->value];
                }
                if(temp_bonus > bonus)
                {
                    bonus = temp_bonus;
                    current_reg_id = reg_id;
                }
            }
            remained_all_freg_ids.erase(current_reg_id);
            current->reg_id = current_reg_id;
            active.insert(current);
            freg2ActInter[current_reg_id].insert(current);
            unused_freg_id.erase(current_reg_id);
        }
        else
        {
            std::set<int, cmp_freg> spare_freg_ids = {};
            for(auto &pair:freg2ActInter)
            {
                bool is_spare = true;
                for(auto &interval:pair.second)
                {
                    if(interval->intersects(current))
                    {
                        is_spare = false;
                        break;
                    } 
                }
                if(is_spare)
                {
                    spare_freg_ids.insert(pair.first);
                }
            }
            if(!spare_freg_ids.empty())
            {
                int current_reg_id = *spare_freg_ids.begin();
                double bonus = -1;
                for(auto &reg_id:spare_freg_ids)
                {
                    double temp_bonus = 0;
                    auto caller_it = caller_arg_bonus[current->value].find(reg_id);
                    if (caller_it != caller_arg_bonus[current->value].end()) {
                        temp_bonus += caller_it->second;
                    }

                    auto callee_it = callee_arg_bonus[current->value].find(reg_id);
                    if (callee_it != callee_arg_bonus[current->value].end()) {
                        temp_bonus += callee_it->second;
                    }

                    for(auto &phi:phi_bonus[current->value])
                    {
                        if(fval2Inter.find(phi.first) != fval2Inter.end())
                        {
                            if(!fval2Inter[phi.first]->intersects(current))
                            {
                                if(fval2Inter[phi.first]->reg_id == reg_id)
                                {
                                    temp_bonus += phi.second;
                                }
                            }
                        }
                    }
                    if(reg_id == 0)
                    {
                        temp_bonus += ret_bonus[current->value];
                        temp_bonus += call_bonus[current->value];
                    }
                    if(temp_bonus > bonus)
                    {
                        bonus = temp_bonus;
                        current_reg_id = reg_id;
                    }
                }
                current->reg_id = current_reg_id;
                active.insert(current);
                freg2ActInter[current_reg_id].insert(current);
                unused_freg_id.erase(current_reg_id);
            }

            auto spill_val = current;
            auto min_expire_val = spill_cost[current->value];
            int spilled_reg_id = -1;
            for(const auto& pair:freg2ActInter){
                double cur_expire_val = 0.;
                for(auto interval:pair.second){
                    if(interval->intersects(current)){
                        cur_expire_val += spill_cost[interval->value];
                    }
                }
                if(cur_expire_val < min_expire_val){
                    spilled_reg_id = pair.first;
                    min_expire_val = cur_expire_val;
                    spill_val = nullptr;
                }
            }
            if(spill_val == current)
            {
                current->reg_id = -1;
            }
            else
            {
                if(spilled_reg_id < 0){
                    LOG(ERROR) << "spilled reg id is -1,something was wrong while register allocation";
                }
                std::set<Interval *> to_spill_set;
                current->reg_id = spilled_reg_id;
                unused_freg_id.erase(spilled_reg_id);
                for(auto inter:freg2ActInter.at(spilled_reg_id)){
                    if(inter->intersects(current)){
                        to_spill_set.insert(inter);
                    }
                }
                for(auto spill_inter:to_spill_set){
                    spill_inter->reg_id = -1;
                    active.erase(spill_inter);
                    freg2ActInter[spilled_reg_id].erase(spill_inter);
                }
                freg2ActInter[spilled_reg_id].insert(current);
                active.insert(current);
            }
        }
    }
    else
    {
        if(!remained_all_ireg_ids.empty())
        {
            double bonus = -1;
            int current_reg_id = *remained_all_ireg_ids.begin();
            for(auto &reg_id:remained_all_ireg_ids)
            {
                double temp_bonus = 0;
                auto caller_it = caller_arg_bonus[current->value].find(reg_id);
                if (caller_it != caller_arg_bonus[current->value].end()) {
                    temp_bonus += caller_it->second;
                }

                auto callee_it = callee_arg_bonus[current->value].find(reg_id);
                if (callee_it != callee_arg_bonus[current->value].end()) {
                    temp_bonus += callee_it->second;
                }

                for(auto &phi:phi_bonus[current->value])
                {
                    if(ival2Inter.find(phi.first) != ival2Inter.end())
                    {
                        if(!ival2Inter[phi.first]->intersects(current))
                        {
                            if(ival2Inter[phi.first]->reg_id == reg_id)
                            {
                                temp_bonus += phi.second;
                            }
                        }
                    }
                }
                if(reg_id == 4)
                {
                    temp_bonus += ret_bonus[current->value];
                    temp_bonus += call_bonus[current->value];
                }
                if(temp_bonus > bonus)
                {
                    bonus = temp_bonus;
                    current_reg_id = reg_id;
                }
            }
            remained_all_ireg_ids.erase(current_reg_id);
            current->reg_id = current_reg_id;
            active.insert(current);
            LOG_DEBUG << "current_reg_id: " << current_reg_id;
            ireg2ActInter[current_reg_id].insert(current);
            unused_ireg_id.erase(current_reg_id);
        }
        else
        {
            std::set<int, cmp_ireg> spare_ireg_ids = {};
            for(auto &pair:ireg2ActInter)
            {
                bool is_spare = true;
                for(auto &interval:pair.second)
                {
                    if(interval->intersects(current))
                    {
                        is_spare = false;
                        break;
                    } 
                }
                if(is_spare)
                {
                    spare_ireg_ids.insert(pair.first);
                }
            }
            if(!spare_ireg_ids.empty())
            {
                int current_reg_id = *spare_ireg_ids.begin();
                double bonus = -1;
                for(auto &reg_id:spare_ireg_ids)
                {
                    double temp_bonus = 0;
                    auto caller_it = caller_arg_bonus[current->value].find(reg_id);
                    if (caller_it != caller_arg_bonus[current->value].end())
                    {
                        temp_bonus += caller_it->second;
                    }
                    auto callee_it = callee_arg_bonus[current->value].find(reg_id);
                    if (callee_it != callee_arg_bonus[current->value].end())
                    {
                        temp_bonus += callee_it->second;
                    }

                    for(auto &phi:phi_bonus[current->value])
                    {
                        if(ival2Inter.find(phi.first) != ival2Inter.end())
                        {
                            if(!ival2Inter[phi.first]->intersects(current))
                            {
                                if(ival2Inter[phi.first]->reg_id == reg_id)
                                {
                                    temp_bonus += phi.second;
                                }
                            }
                        }
                    }
                    if(reg_id == 0)
                    {
                        temp_bonus += ret_bonus[current->value];
                        temp_bonus += call_bonus[current->value];
                    }
                    if(temp_bonus > bonus)
                    {
                        bonus = temp_bonus;
                        current_reg_id = reg_id;
                    }
                }
                current->reg_id = current_reg_id;
                active.insert(current);
                LOG_DEBUG << "current_reg_id: " << current_reg_id;
                ireg2ActInter[current_reg_id].insert(current);
                unused_ireg_id.erase(current_reg_id);
            }

            auto spill_val = current;
            auto min_expire_val = spill_cost[current->value];
            int spilled_reg_id = -1;
            for(const auto& pair:ireg2ActInter)
            {
                LOG_DEBUG << "ireg2ActInter: " << pair.first;
                double cur_expire_val = 0.;
                for(auto interval:pair.second)
                {
                    if(interval->intersects(current))
                    {
                        LOG_DEBUG << "interval: " << interval->value->get_name() << " " << spill_cost[interval->value];
                        cur_expire_val += spill_cost[interval->value];
                        LOG_DEBUG << "cur_expire_val: " << cur_expire_val << " min_expire_val: " << min_expire_val;
                    }
                }
                if(cur_expire_val < min_expire_val)
                {
                    LOG_DEBUG << "cur_expire_val: " << cur_expire_val << " min_expire_val: " << min_expire_val ;
                    spilled_reg_id = pair.first;
                    min_expire_val = cur_expire_val;
                    spill_val = nullptr;
                }
            }
            if(spill_val == current)
            {
                current->reg_id = -1;
            }
            else
            {
                if(spilled_reg_id < 0)
                {
                    LOG(ERROR) << "spilled reg id is -1,something was wrong while register allocation" ;
                    //<< spill_val->value->print()
                    LOG_ERROR << current->value->print();
                }
                std::set<Interval *> to_spill_set;
                current->reg_id = spilled_reg_id;
                unused_ireg_id.erase(spilled_reg_id);
                for(auto inter:ireg2ActInter[spilled_reg_id])
                {
                    if(inter->intersects(current))
                    {
                        to_spill_set.insert(inter);
                    }
                }
                for(auto spill_inter:to_spill_set)
                {
                    spill_inter->reg_id = -1;
                    active.erase(spill_inter);
                    ireg2ActInter[spilled_reg_id].erase(spill_inter);
                }
                LOG_DEBUG << "spilled_reg_id: " << spilled_reg_id;
                ireg2ActInter[spilled_reg_id].insert(current);
                active.insert(current);
            }
        }
    }
}
void RegAlloc::add_reg_to_pool(Interval *interval)
{
    auto reg_id = interval->reg_id;
    if(interval->is_float_type())
    {
        if(reg_id < 0 || reg_id > 23)
        {
            return;
        }
        if(freg2ActInter[reg_id].size() <= 1)
        {
            remained_all_freg_ids.insert(reg_id);
        }
        freg2ActInter[reg_id].erase(interval);
    }
    else
    {
        if(reg_id < 4 || reg_id > 20)
        {
            return;
        }
        if(ireg2ActInter[reg_id].size() <= 1)
        {
            remained_all_ireg_ids.insert(reg_id);
        }
        ireg2ActInter[reg_id].erase(interval);
    }
}
void RegAlloc::compute_block_order()
{
    std::priority_queue<BasicBlock*, std::vector<BasicBlock*>, cmp_block_depth> block_queue;
    block_order.clear();
    block_queue.push(func->get_entry_block());
    while(!block_queue.empty())
    {
        auto bb = block_queue.top();
        block_queue.pop();
        block_order.push_back(bb);
        for(auto &succ:bb->get_succ_basic_blocks())
        {
            succ->indegree_add(-1);
            if(succ->indegree_is_zero())
            {
                block_queue.push(succ);
            }
        }
    }
}

void RegAlloc::number_operations()
{
    int next_id = 0;
    for(auto &bb:block_order)
    {
        for(auto &inst:bb->get_instructions())
        {
            inst.set_id(next_id);
            next_id += 2;
        }
    }
}

void RegAlloc::build_intervals()
{
    for(auto bb_iter = block_order.rbegin();bb_iter != block_order.rend();bb_iter ++)
    {
        LOG_DEBUG << "BasicBlock: " << (*bb_iter)->get_name();
        auto bb = *bb_iter;
        auto &bb_instrs = bb->get_instructions();
        int block_from = bb_instrs.begin()->get_id();
        int block_to = bb_instrs.rbegin()->get_id() + 2;
        for(auto &live_out_int : bb->get_live_out_int())
        {
           // LOG_DEBUG << "live_out_int: " << live_out_int->get_name();
            if((!dynamic_cast<Instruction *>(live_out_int) && !dynamic_cast<Argument *>(live_out_int)) || dynamic_cast<AllocaInst *>(live_out_int))
            {
                continue;
            }
            if(ival2Inter.find(live_out_int) == ival2Inter.end())
            {
                ival2Inter[live_out_int] = new Interval(live_out_int);
            }
            ival2Inter[live_out_int]->add_range(block_from, block_to);
        }
        for(auto &live_out_float : bb->get_live_out_float())
        {
            if((!dynamic_cast<Instruction *>(live_out_float) && !dynamic_cast<Argument *>(live_out_float)) || dynamic_cast<AllocaInst *>(live_out_float))
            {
                continue;
            }
            if(fval2Inter.find(live_out_float) == fval2Inter.end())
            {
                fval2Inter[live_out_float] = new Interval(live_out_float);
            }
            fval2Inter[live_out_float]->add_range(block_from, block_to);
        }
        for(auto inst_iter = bb_instrs.rbegin();inst_iter != bb_instrs.rend();inst_iter++)
        {
            auto inst = &*inst_iter;
            if(!inst->is_void())
            {
                if(inst->is_alloca()) continue;
                if(inst->get_type()->is_float_type())
                {
                    if(fval2Inter.find(inst) == fval2Inter.end())
                    {
                        fval2Inter[inst] = new Interval(inst);
                        fval2Inter[inst]->add_range(block_from, block_to);
                    }
                    fval2Inter[inst]->range_list.front()->from = inst->get_id();
                    fval2Inter[inst]->add_use_pos(inst->get_id());
                }
                else {
                    if(ival2Inter.find(inst) == ival2Inter.end())
                    {
                        ival2Inter[inst] = new Interval(inst);
                        ival2Inter[inst]->add_range(block_from, block_to);
                    }
                    ival2Inter[inst]->range_list.front()->from = inst->get_id();
                    ival2Inter[inst]->add_use_pos(inst->get_id());
                }
            }
            //if(inst->is_phi()) continue;
            for(auto &op:inst->get_operands())
            {
                if((!dynamic_cast<Instruction *>(op) && !dynamic_cast<Argument *>(op)) || dynamic_cast<AllocaInst *>(op))
                {
                    continue;
                }
                if(op->get_type()->is_float_type())
                {
                    if(fval2Inter.find(op) == fval2Inter.end())
                    {
                        fval2Inter[op] = new Interval(op);
                    }
                    fval2Inter[op]->add_range(block_from, inst->get_id() + 2);
                    fval2Inter[op]->add_use_pos(inst->get_id());
                }
                else
                {
                    if(ival2Inter.find(op) == ival2Inter.end())
                    {
                        ival2Inter[op] = new Interval(op);
                    }
                   // LOG_DEBUG << "op2587: " << op->get_name();
                    ival2Inter[op]->add_range(block_from, inst->get_id() + 2);
                    ival2Inter[op]->add_use_pos(inst->get_id());
                }
            }
        }
    }
    for(auto &ival:ival2Inter)
    {
        add_interval(ival.second);
    }
    for(auto &fval:fval2Inter)
    {
        add_interval(fval.second);
    }
}

void RegAlloc::compute_bonus_and_cost()
{
    int iarg_count = 0;
    int farg_count = 0;
    for(auto &arg:func->get_args())
    {
        if(arg.get_type()->is_float_type())
        {
            if(farg_count < 8)
            {
                callee_arg_bonus[&arg][farg_count] += mov_cost;
                spill_cost[&arg] += store_cost; 
            }
            farg_count++;
        }
        else
        {
            if(iarg_count < 8)
            {
                callee_arg_bonus[&arg][iarg_count] += mov_cost;
                spill_cost[&arg] += store_cost;
            }
            iarg_count++;
        }
    }
    for(auto &bb : block_order)
    {
        auto loop_depth = bb->get_loop_depth();
        auto scale = std::pow(loop_scale, loop_depth);
        for(auto &inst:bb->get_instructions())
        {
            if(inst.is_phi())
            {
                for(auto i = 0;i < inst.get_num_operand();i++)
                {
                    if(dynamic_cast<Constant *>(inst.get_operand(i))|| dynamic_cast<GlobalVariable *>(inst.get_operand(i)) || dynamic_cast<Function *>(inst.get_operand(i)) || dynamic_cast<BasicBlock *>(inst.get_operand(i)))
                    {
                        continue;
                    }
                    phi_bonus[&inst][inst.get_operand(i)] += scale * mov_cost;
                    phi_bonus[inst.get_operand(i)][&inst] += scale * mov_cost;
                }
            }

            if(inst.is_call())
            {
                int iarg_count = 0;
                int farg_count = 0;
                for(auto i = 1;i < inst.get_num_operand();i++)
                {
                    auto op = inst.get_operand(i);
                    if(op->get_type()->is_float_type())
                    {
                        if(farg_count < 8)
                        {
                            caller_arg_bonus[&inst][farg_count] += mov_cost;
                        }
                        farg_count++;
                    }
                    else
                    {
                        if(iarg_count < 8)
                        {
                            caller_arg_bonus[&inst][iarg_count] += mov_cost;
                        }
                        iarg_count++;
                    }
                }
                if(!inst.is_void())
                {
                    call_bonus[&inst] += mov_cost * scale;
                }
            }

            if(inst.is_ret())
            {
                if(inst.get_num_operand() != 0)
                {
                    ret_bonus[inst.get_operand(0)] += mov_cost;
                }
            }

            if(!inst.is_alloca() && !inst.is_gep())
            {
                for(auto &op:inst.get_operands())
                {
                    if(dynamic_cast<Constant *>(op) || dynamic_cast<GlobalVariable *>(op) || dynamic_cast<Function *>(op) || dynamic_cast<BasicBlock *>(op))
                    {
                        continue;
                    }
                    spill_cost[op] += load_cost * scale;
                }
            }
            if(inst.is_gep())
            {
                for(auto op:inst.get_operands())
                { 
                    if(dynamic_cast<GlobalVariable*>(op)||dynamic_cast<AllocaInst*>(op)||dynamic_cast<Constant*>(op))
                        continue;
                    spill_cost[op] += load_cost * scale;
                }
            }

            if(!inst.is_void())
            {
                if(!inst.is_alloca())
                {
                    spill_cost[&inst] += store_cost * scale;
                }
            }
        }            
    }
}

void ActiveVar::run()
{
    for(auto &func:m->get_functions())
    {
        if(func.is_declaration())
        {
            continue;
        }
        ilive_in.clear();
        ilive_out.clear();
        flive_in.clear();
        flive_out.clear();
        def_map.clear();
        use_map.clear();
        phi_map.clear();
        LOG_DEBUG << "Function: " << func.get_name();
        get_def_use(&func);
        get_in_out(&func);
        for(auto &bb:func.get_basic_blocks())
        {
            ilive_in1[&bb] = ilive_in[&bb];
            ilive_out1[&bb] = ilive_out[&bb];
            flive_in1[&bb] = flive_in[&bb];
            flive_out1[&bb] = flive_out[&bb];
            def_map1[&bb] = def_map[&bb];
            use_map1[&bb] = use_map[&bb];
            phi_map1[&bb] = phi_map[&bb];
        }
    }
   /*for(auto &func:m->get_functions())
    {
        LOG_DEBUG << "Function: " << func.get_name();
        for(auto &bb:func.get_basic_blocks())
        {
            LOG_DEBUG << "BasicBlock: " << bb.get_name();
           LOG_DEBUG << "def:";
            for(auto &def:def_map1[&bb])
            {
                LOG_DEBUG << def->get_name();
            }
            LOG_DEBUG << "use:";
            for(auto &use:use_map1[&bb])
            {
                LOG_DEBUG << use->get_name();
            }
            LOG_DEBUG << "phi:";
            for(auto &phi:phi_map1[&bb])
            {
                LOG_DEBUG << phi->get_name();
            }
            LOG_DEBUG << "ilive_in:";
            for(auto &val:ilive_in1[&bb])
            {
                LOG_DEBUG << val->get_name();
            }
            LOG_DEBUG << "ilive_out:";
            for(auto &val:ilive_out1[&bb])
            {
                LOG_DEBUG << val->get_name();
            }
            LOG_DEBUG << "flive_in:";
            for(auto &val:flive_in[&bb])
            {
                LOG_DEBUG << val->get_name();
            }
            LOG_DEBUG << "flive_out:";
            for(auto &val:flive_out[&bb])
            {
                LOG_DEBUG << val->get_name();
            }
        }
    }
    */ 
    LOG_DEBUG << "Start Iterative Dataflow Analysis";
    for(auto &func:m->get_functions())
    {
        for(auto &bb: func.get_basic_blocks())
        {
            bb.set_live_in_float(flive_in1[&bb]);
            bb.set_live_out_float(flive_out1[&bb]);
            bb.set_live_in_int(ilive_in1[&bb]);
            bb.set_live_out_int(ilive_out1[&bb]);
            bb.set_def(def_map1[&bb]);
            bb.set_use(use_map1[&bb]);
        }
    }
}

void ActiveVar::get_def_use(Function *func)
{
    for(auto &bb:func->get_basic_blocks())
    {
        def_map[&bb] = std::set<Value*>();
        use_map[&bb] = std::set<Value*>();
        phi_map[&bb] = std::set<Value*>();
        for(auto &inst:bb.get_instructions())
        {
            if(!inst.is_void())
            {
               // LOG_DEBUG << "Instruction: " << inst.get_name();
                def_map[&bb].insert(&inst);
                if(inst.is_phi())
                {
                    phi_map[&bb].insert(&inst); 
                }
            }
            if(!inst.is_phi())
            {
                for(auto &op:inst.get_operands())
                {
                    if(dynamic_cast<Constant *>(op) || dynamic_cast<GlobalVariable *>(op) || dynamic_cast<Function *>(op) || dynamic_cast<BasicBlock *>(op))
                    {
                        continue;
                    }
                    if(def_map[&bb].find(op) == def_map[&bb].end())
                    {
                        use_map[&bb].insert(op);
                    }
                }
            }
        }
    }
}

void ActiveVar::get_in_out(Function *func)
{
    for(auto &bb:func->get_basic_blocks())
    {
        ilive_in[&bb] = std::set<Value*>();
        ilive_out[&bb] = std::set<Value*>();
        flive_in[&bb] = std::set<Value*>();
        flive_out[&bb] = std::set<Value*>();
        for(auto &use:use_map[&bb])
        {
            if(use->get_type()->is_float_type())
            {
                flive_in[&bb].insert(use);
            }
            else
            {
                ilive_in[&bb].insert(use);
            }
        }
    }
    bool changed = true;
    while(changed)
    {
        changed = false;
        for(auto &bb:func->get_basic_blocks())
        {
            //OUT[B] = U S是B的后继 IN [S]
            //IN [B] = useB U (OUT [B] − def[B])；
            for(auto &succ_bb:bb.get_succ_basic_blocks())
            {
             //   LOG_DEBUG << "BasicBlock: " << bb.get_name() << " succ: " << succ_bb->get_name();
                for(auto &val:ilive_in[succ_bb])
                {
                    if(phi_map[succ_bb].find(val) != phi_map[succ_bb].end())
                        continue;
                //    LOG_DEBUG << "val: " << val->get_name();
                    if(ilive_out[&bb].find(val) == ilive_out[&bb].end())
                    {
                        ilive_out[&bb].insert(val);
                        changed = true;
                        if(def_map[&bb].find(val) == def_map[&bb].end())
                        {
                            ilive_in[&bb].insert(val);
                        }
                    }
                }
                for(auto &val:flive_in[succ_bb])
                {
                    if(phi_map[succ_bb].find(val) != phi_map[succ_bb].end())
                        continue;
                    if(flive_out[&bb].find(val) == flive_out[&bb].end())
                    {
                        flive_out[&bb].insert(val);
                        changed = true;
                        if(def_map[&bb].find(val) == def_map[&bb].end())
                        {
                            flive_in[&bb].insert(val);
                        }
                    }
                }
                for(auto &val:phi_map[succ_bb])
                {
                    auto phi = dynamic_cast<PhiInst *>(val);
                    for(auto i = 0;i < phi->get_num_operand();i+=2)
                    {
                        auto op = phi->get_operand(i);
                        auto bb1 = dynamic_cast<BasicBlock *>(phi->get_operand(i+1));
                        if(bb1 == &bb)
                        {
                            if(op->get_type()->is_float_type())
                            {
                                if(flive_out[&bb].find(op) == flive_out[&bb].end())
                                {
                                    flive_out[&bb].insert(op);
                                    changed = true;
                                    if(def_map[&bb].find(op) == def_map[&bb].end())
                                    {
                                        flive_in[&bb].insert(op);
                                    }
                                }
                            }
                            else
                            {
                                if(ilive_out[&bb].find(op) == ilive_out[&bb].end())
                                {
                                    ilive_out[&bb].insert(op);
                                    changed = true;
                                    if(def_map[&bb].find(op) == def_map[&bb].end())
                                    {
                                        ilive_in[&bb].insert(op);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    for(auto &bb:func->get_basic_blocks())
    {
        for(auto &phi:phi_map[&bb])
        {
            auto phi_inst = dynamic_cast<PhiInst *>(phi);
            for(auto i = 0;i < phi_inst->get_num_operand();i+=2)
            {
                auto op = phi_inst->get_operand(i);
                if(op->get_type()->is_float_type())
                {
                    if(flive_in[&bb].find(op) != flive_out[&bb].end())
                    {
                        flive_in[&bb].erase(op);
                    }
                }
                else
                {
                    if(ilive_in[&bb].find(op) != ilive_out[&bb].end())
                    {
                        ilive_in[&bb].erase(op);
                    }
                }
            }
        }
    }
}

