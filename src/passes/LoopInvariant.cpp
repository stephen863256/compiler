#include "LoopInvariant.hpp"
#include "BasicBlock.hpp"
#include "Function.hpp"
#include "Instruction.hpp"
#include "Value.hpp"
#include "logging.hpp"
#include <string>

void LoopInvariant::run() {
    loop_find = std::make_unique<LoopFind>(module_);
    loop_find->run();   
    auto loops = loop_find->get_loops();
    for(auto &loop: *loops) {
        bool is_inner_loop = true;
        for(auto &bb: *loop) {
            if(loop_find->get_bb_loop(bb) != loop) {
                is_inner_loop = false;
                break;
            }
        }
        if(is_inner_loop) {
            auto loop_ = loop;
          //  LOG_DEBUG << "find inner loop  " << loop_find->get_loop_entry(loop)->get_name();
            while(loop_ != nullptr) {
                loop_invariant.clear();
                find_loop_invariant(loop_);
                if(!loop_invariant.empty()) {
                    move_invariant_out(loop_);
                }
                loop_ = loop_find->get_parent_loop(loop_);
            }
        }
    }
   // LOG_DEBUG << "LoopInvariant done";
  //  LOG_DEBUG << module_->print();
}

void LoopInvariant::find_loop_invariant(Loop *loop) {
    std::set<Value *> loop_def;
    std::vector<Instruction *> bb_invariant;
    for(auto &bb: *loop) {
        for(auto &inst: bb->get_instructions()) {
            loop_def.insert(&inst);
        }
    }
    bool changed = false;
    do
    {
        changed = false;
        for(auto &bb: *loop) {
            bb_invariant.clear();
            for(auto &inst: bb->get_instructions()) {
                bool is_invariant = true;
                if(inst.is_call() || inst.is_phi() || inst.is_br() || inst.is_ret() || inst.is_alloca() || inst.is_store()
                || inst.is_cmp() || inst.is_fcmp())
                    continue;
                if(loop_def.find(&inst) == loop_def.end()) 
                    continue;
             //   LOG_DEBUG << "check loop invariant: " << inst.print();
                for(auto &op: inst.get_operands()) {
                    if(loop_def.find(op) != loop_def.end()) {
                        is_invariant = false;
                        break;
                    }
                }
                if(inst.is_load() && is_invariant)
                {
                    for(auto &bb: *loop){
                    for(auto &inst_:bb->get_instructions())
                    {
                      //  if(&inst_ == &inst)
                        //    break;
                        if(inst_.is_store())
                        {
                            auto store = dynamic_cast<StoreInst *>(&inst_);
                            auto load = dynamic_cast<LoadInst *>(&inst);
                            if(store->get_lval() == load->get_lval())
                            {
                                is_invariant = false;
                                break;
                            }
                        }
                        else if(inst_.is_call())
                        {
                            auto func = dynamic_cast<Function *>(inst_.get_operand(0));
                            std::vector<Value *> args;
                            for(int i = 1; i < inst_.get_num_operand(); i++)
                            {
                                args.push_back(inst_.get_operand(i));
                            }
                           // is_invariant = after_call_is_invariant(func,args,dynamic_cast<LoadInst *>(&inst));
                            is_invariant = false;
                            break;
                        }
                    }
                    }
                }
                if(is_invariant) {
                    bb_invariant.push_back(&inst);
                    loop_def.erase(&inst);
                    changed = true;
                }
            }
            if(!bb_invariant.empty()) {
                loop_invariant.push_back(std::make_pair(bb,bb_invariant));
            }
        }
    }while(changed);
}
auto cnt = 0;
void LoopInvariant::move_invariant_out(Loop *loop) {
    auto first_bb = loop_find->get_loop_entry(loop);
    BasicBlock *new_bb = BasicBlock::create(module_,"new_bb"+ std::to_string(cnt ++),first_bb->get_parent());
    for(auto &inst: first_bb->get_instructions()) {
        if(!inst.is_phi()) {
            continue;
        }
        auto phi = dynamic_cast<PhiInst *>(&inst);
        std::vector<std::pair<Value *, BasicBlock*>> def_in_loop;
        std::vector<std::pair<Value *, BasicBlock *>> def_out_loop;
        for(int i = 0; i < phi->get_num_operand(); i += 2) {
            auto bb = dynamic_cast<BasicBlock *>(phi->get_operand(i + 1));
            auto val = phi->get_operand(i);
            if(loop_find->get_bb_loop(bb) == loop) {
             //   LOG_DEBUG << "def in loop: " << val->get_name() << "   " << bb->get_name();
                def_in_loop.push_back(std::make_pair(val, bb));
            }
            else {
             //   LOG_DEBUG << "def out loop: " << val->get_name() << "   " << bb->get_name();
                def_out_loop.push_back(std::make_pair(val, bb));
            }       
        }
        if(def_out_loop.size() > 1){
            auto new_phi = PhiInst::create_phi(phi->get_type(), new_bb);
            for(auto &def: def_out_loop) {
                new_phi->add_phi_pair_operand(def.first,def.second); 
            }
            phi->remove_all_operands();
            for(auto &def: def_in_loop) {
                phi->add_phi_pair_operand(def.first,def.second);
            }
            phi->add_phi_pair_operand(new_phi,new_bb);
        }
        else {
            phi->remove_all_operands();
            for(auto &def: def_in_loop) {
             //   LOG_DEBUG << "add phi pair operand: " << def.first->get_name() << "   " << def.second->get_name();
                phi->add_phi_pair_operand(def.first,def.second);
            }
            for(auto &def: def_out_loop) {
             //   LOG_DEBUG << "add phi pair operand: " << def.first->get_name() << "   " << new_bb->get_name();
                phi->add_phi_pair_operand(def.first,new_bb);
            }   
         //   LOG_DEBUG << "phi operand size" << phi->get_num_operand();
        }
    }
    //LOG_DEBUG << first_bb->print();
    for(auto &invariant : loop_invariant) {
        auto bb = invariant.first;
        for(auto &inst: invariant.second) {
            bb->get_instructions().remove(inst);
           //LOG_DEBUG << "move invariant out: " << inst->get_name();
            new_bb->add_instruction(inst);
            inst->set_parent(new_bb);
        }
    }
    
    std::vector<BasicBlock *> pre_bbs;
    for(auto &pre_bb: first_bb->get_pre_basic_blocks()) {
        if(loop_find->get_bb_loop(pre_bb) != loop) 
        {
            pre_bbs.push_back(pre_bb);
        }
    }
    auto new_br = BranchInst::create_br(first_bb,new_bb);
   // LOG_DEBUG << "new_bb: " << new_bb->print() << " "<< new_br->print();
    for(auto &pre_bb: pre_bbs) {
          //  LOG_DEBUG << "pre_bb: " << pre_bb->get_name();
            auto term = pre_bb->get_terminator();
            if(term->is_br()) {
                auto br = dynamic_cast<BranchInst *>(term);
                if(br->is_cond_br())
                {
                    if(br->get_operand(1) == first_bb)
                    {
                        pre_bb->get_instructions().remove(br);
                        //pre_bb->remove_succ_basic_block(first_bb);
                        //first_bb->remove_pre_basic_block(pre_bb);
                        auto false_bb = dynamic_cast<BasicBlock *>(br->get_operand(2));
                        BranchInst::create_cond_br(br->get_operand(0),new_bb,false_bb,pre_bb);
                    }
                    else if(br->get_operand(2) == first_bb)
                    {
                        pre_bb->get_instructions().remove(br);
                        //pre_bb->remove_succ_basic_block(first_bb);
                        //first_bb->remove_pre_basic_block(pre_bb);
                        auto true_bb = dynamic_cast<BasicBlock *>(br->get_operand(1));
                        BranchInst::create_cond_br(br->get_operand(0),true_bb,new_bb,pre_bb);
                        //pre_bb->add_instruction(br);
                    }
                }
                else {
                   // LOG_DEBUG << "br: " << br->print();
                    pre_bb->get_instructions().remove(br);
                    //pre_bb->remove_succ_basic_block(first_bb);
                    //first_bb->remove_pre_basic_block(pre_bb);
                    auto new_br = BranchInst::create_br(new_bb,pre_bb);
                  // LOG_DEBUG << "new_br: " << new_br->print();
                    //pre_bb->add_instruction(new_br);
                }
                pre_bb->remove_succ_basic_block(first_bb);
                first_bb->remove_pre_basic_block(pre_bb);
            }
        
    }
}
bool LoopInvariant::after_call_is_invariant(Function *func,std::vector<Value *> args,LoadInst * inst) {
    for(auto &arg: args) {
        auto gep1 = dynamic_cast<GetElementPtrInst *>(arg);
        auto gep2 = dynamic_cast<GetElementPtrInst *>(inst->get_lval());
        if(gep1 != nullptr && gep2 != nullptr) {
            if(gep1->get_operand(0) == gep2->get_operand(0))
                return false;
        }
    }
    std::stack<Function *> call_stack;
    call_stack.push(func);
    while(!call_stack.empty()) {
        auto func = call_stack.top();
        call_stack.pop();
        //auto func = call.first;
        //auto args = call.second;
        for(auto &bb: func->get_basic_blocks()) {
            for(auto &inst_: bb.get_instructions()) {
                if(inst_.is_call()) {
                    auto func = dynamic_cast<Function *>(inst_.get_operand(0));
                  //  std::vector<Value *> args;
                   // for(int i = 1; i < inst_.get_num_operand(); i++) {
                    //    args.push_back(inst_.get_operand(i));
                    //}
                    call_stack.push(func);
                }
                else if(inst_.is_store()) {
                    auto store = dynamic_cast<StoreInst *>(&inst_);
                    auto lval = store->get_lval();
                    if(lval == inst->get_lval()) {
                        return false;
                    }
                }
            }
        }
    }
    return true;
}