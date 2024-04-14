#include "ConstProp.hpp"
#include "BasicBlock.hpp"
#include "Constant.hpp"
#include "Dominators.hpp"
#include "IRBuilder.hpp"
#include "Instruction.hpp"
#include "Value.hpp"
#include "logging.hpp"


#define CONST_INT(val1,val2,op) (ConstantInt::get(get_const_int_val(val1) op get_const_int_val(val2), module_))
#define CONST_FP_TO_INT(val1,val2,op) (ConstantInt::get(get_const_float_val(val1) op get_const_float_val(val2), module_))
#define CONST_FLOAT(val1,val2,op) (ConstantFP::get(get_const_float_val(val1) op get_const_float_val(val2), module_))
int get_const_int_val(Value *val) {
    if (auto *const_val = dynamic_cast<ConstantInt *>(val)) {
        return const_val->get_value();
    }
    return -1;
}

float get_const_float_val(Value *val) {
    if (auto *const_val = dynamic_cast<ConstantFP *>(val)) {
        return const_val->get_value();
    }
    return -1;
}

Constant *ConstFolder::compute(Instruction *instr, Constant *value1, Constant *value2)
{
    switch (instr->get_instr_type()) {
        case Instruction::add:  return CONST_INT(value1, value2, +);
        case Instruction::sub:  return CONST_INT(value1, value2, -);
        case Instruction::mul:  return CONST_INT(value1, value2, *);
        case Instruction::sdiv: return CONST_INT(value1, value2, /);
        case Instruction::fadd: return CONST_FLOAT(value1, value2, +);
        case Instruction::fsub: return CONST_FLOAT(value1, value2, -);
        case Instruction::fmul: return CONST_FLOAT(value1, value2, *);
        case Instruction::fdiv: return CONST_FLOAT(value1, value2, /);
        case Instruction::srem: return CONST_INT(value1, value2, %);
        case Instruction::ge: return CONST_INT(value1, value2, >=);
        case Instruction::gt: return CONST_INT(value1, value2, >);
        case Instruction::le: return CONST_INT(value1, value2, <=);
        case Instruction::lt: return CONST_INT(value1, value2, <);
        case Instruction::eq: return CONST_INT(value1, value2, ==);
        case Instruction::ne: return CONST_INT(value1, value2, !=);
        case Instruction::fge: return CONST_FP_TO_INT(value1, value2, >=);
        case Instruction::fgt: return CONST_FP_TO_INT(value1, value2, >);
        case Instruction::fle: return CONST_FP_TO_INT(value1, value2, <=);
        case Instruction::flt: return CONST_FP_TO_INT(value1, value2, <);
        case Instruction::feq: return CONST_FP_TO_INT(value1, value2, ==);
        default: return nullptr;
    }
}

Constant *ConstFolder::compute(Instruction *instr, Constant *value)
{
    switch (instr->get_instr_type()) {
        case Instruction::sitofp: return ConstantFP::get((float)get_const_int_val(value), module_);
        case Instruction::fptosi: return ConstantInt::get((int)get_const_float_val(value), module_);
        case Instruction::zext: return ConstantInt::get(get_const_int_val(value), module_);
        default: return nullptr;
    }
}

void ConstProp::run() {
   
    unchanged_global_vars.clear();

    find_unchanged_global_vars();
    Dominators dom(m_);
    for (auto &F : m_->get_functions()) {
        dead_blocks.clear();
        auto func = &F;
        if(func->get_num_basic_blocks() == 0)
            continue;
        dom.create_reverse_post_order(func);
      //   LOG_INFO << module_->print();
       // for(auto &bb : dom.get_reverse_post_order())
        //{
        //    LOG_DEBUG << "const fold bb: " << bb->get_name();
        //}
     //   LOG_DEBUG << "const fold " << func->get_name();
        for(auto &bb : dom.get_reverse_post_order())
        {
         //   LOG_DEBUG << "const fold bb: " << bb->get_name();
            const_fold(bb);
        }
        for(auto &bb : dom.get_reverse_post_order())
        {
            dead_blocks.insert(bb);
        }
        find_dead_block(func,func->get_entry_block());
     //   LOG_DEBUG << "dead_blocks size: " << dead_blocks.size();
        for(auto &bb : dead_blocks)
        {
         //   LOG_DEBUG << "dead_blocks: " << bb->get_name();
        }
       // LOG_INFO << module_->print();
        clear_dead_block(func);
     //   LOG_DEBUG << "remove single phi";
        remove_single_phi(func);
    }
}

void ConstProp::find_unchanged_global_vars() {
    for (auto &gv : m_->get_global_variable())
    {
        if(unchanged_global_vars.find(&gv) == unchanged_global_vars.end())
        {
            unchanged_global_vars.insert(&gv);
        }
    }
    for(auto &F : m_->get_functions())
    {
        auto func = &F;
        for(auto &bb : func->get_basic_blocks())
        {
            if(func->get_num_basic_blocks() == 0)
                continue;
            for(auto &inst : bb.get_instructions())
            {
                if(inst.is_store())
                {
                    auto lval = inst.get_operand(1);
                    if(dynamic_cast<GlobalVariable *>(lval))
                    {
                        unchanged_global_vars.erase(lval);
                    }
                }
            }
        }
    }
}

void ConstProp::find_dead_block(Function *func,BasicBlock *bb)
{
    dead_blocks.erase(bb);
    
    for(auto sucbb: bb->get_succ_basic_blocks())
    {
        if(dead_blocks.find(sucbb) != dead_blocks.end())
        {
          //  LOG_DEBUG << bb->get_name() << "find not dead block: " << sucbb->get_name();
            find_dead_block(func,sucbb);
        }
    }
}

void ConstProp::clear_dead_block(Function *func)
{
  //  LOG_DEBUG << dead_blocks.size() << "  blocks";
    for(auto &bb : func->get_basic_blocks())
    {
        for(auto &inst : bb.get_instructions())
        {
            if(!inst.is_phi())
            {
                continue;
            }
           // LOG_DEBUG << "clear dead block inst: " << inst.print();
           // std::set<std::pair<Value *,Value *>> phi_values;
            //bool remove_phi = false;
            for(auto i = 1;i < inst.get_num_operand();i += 2)
            {
               // LOG_DEBUG << "clear dead block inst: " << i;
                if(dead_blocks.find(dynamic_cast<BasicBlock *>(inst.get_operand(i))) != dead_blocks.end())
                {
                  //  LOG_DEBUG << "clear dead block phi " << inst.get_operand(i)->get_name();
                    inst.remove_operand(i);
                    inst.remove_operand(i - 1);
                }
            }
           // LOG_DEBUG <<"phi_inst size" << inst.get_num_operand();
        }
    }
   // LOG_INFO << module_->print();
   // LOG_DEBUG << "clear dead block1111111";
    
    for(auto bb:dead_blocks)  
    {
        std::set<Instruction *> dead_inst;
     //   LOG_DEBUG << "clear dead block1111 " << bb->get_name();
        for(auto &inst:bb->get_instructions())
        {
            dead_inst.insert(&inst);
        }
      //  LOG_DEBUG << "finish insert";
        for(auto &inst:dead_inst)
        {
            bb->erase_instr(inst);
        }
    }
    for(auto bb:dead_blocks)
    {
        func->remove(bb);
       // LOG_DEBUG << func->get_num_basic_blocks() << " 1234" << bb->get_name();
    }
}

void ConstProp::remove_single_phi(Function *func)
{
    for(auto &bb : func->get_basic_blocks())
    {
        std::list<Instruction *> dead_inst;
        for(auto &inst : bb.get_instructions())
        {
            if(inst.is_phi())
            {
                auto phi = dynamic_cast<PhiInst *>(&inst);
                if(phi->get_num_operand() == 2)
                {
                    auto val = phi->get_operand(0);
                    phi->replace_all_use_with(val);
                    dead_inst.push_back(phi);
                }
            }
        }
        for(auto &inst : dead_inst)
        {
            bb.erase_instr(inst);
        }
    }
}

void ConstProp::const_fold(BasicBlock *bb)
{
    std::set<Instruction *> dead_inst;
    std::map<Value*, std::map<Value*, Value*>*> array_map;
    std::set<std::pair<Instruction *,BasicBlock *>> br_replace;
    for(auto &inst : bb->get_instructions())
    {
       // LOG_DEBUG << "const fold inst: " << inst.print();
        if(inst.is_phi())
        {
            continue;
        }
        else if(inst.is_call())
        {
          //  LOG_DEBUG << ("=====call=====") << inst.print();
            for(auto i = 1;i < inst.get_num_operand();i++)
            {
                auto op = inst.get_operand(i);
                auto gep = dynamic_cast<GetElementPtrInst *>(op);
                if(gep)
                {
                    op = gep->get_operand(0);
                }
                auto it = array_map.find(op);
                if(it != array_map.end())
                {
                  //  LOG_DEBUG << "erase map";
                    array_map.erase(it);
                }
            }
        }
        else if(inst.is_store())
        {
         //   LOG_DEBUG << ("=====store=====");
            auto store_inst = dynamic_cast<StoreInst *>(&inst);
            auto lval = store_inst->get_lval();
            auto rval = store_inst->get_rval();
            auto gep = dynamic_cast<GetElementPtrInst *>(lval);
            Value *offset = nullptr;
            if(gep)
            {
                lval = gep->get_operand(0);
              //  LOG_DEBUG << "gep: " << gep->print();
                if(gep->get_num_operand() == 2)
                {
                    offset = gep->get_operand(1);
                }
                else if(gep->get_num_operand() == 3)
                {
                    offset = gep->get_operand(2);
                }
                if(array_map.find(lval) == array_map.end())
                {
                 //   LOG_DEBUG << "create new map";
                    auto map = new std::map<Value*, Value*>;
                    map->insert(std::make_pair(offset,rval));
                    array_map.insert(std::make_pair(lval,map));
                }
                else if(array_map[lval]->find(offset) == array_map[lval]->end())
                {
                    array_map[lval]->insert(std::make_pair(offset,rval));
                }
                else
                {
                    array_map[lval]->at(offset) = rval;
                }
            }
        }
        else if(inst.is_load())
        {
         //   LOG_DEBUG << ("=====load=====");
            auto lval = dynamic_cast<LoadInst *>(&inst)->get_lval();
            auto gv = dynamic_cast<GlobalVariable *>(lval);
            auto gep = dynamic_cast<GetElementPtrInst *>(lval);
            if(gv)
            {
                if(unchanged_global_vars.find(lval) != unchanged_global_vars.end())
                {
                    auto val = gv->get_init();  
                    if(gv->is_zero_init())
                    {
                        val = ConstantInt::get(0, module_);
                    }
                    inst.replace_all_use_with(val);
                    dead_inst.insert(&inst);
                }
            }
            else if(gep)
            {
                Value *offset = nullptr;
                lval = gep->get_operand(0);
                if(gep->get_num_operand() == 2)
                {
                    offset = gep->get_operand(1);
                }
                else if(gep->get_num_operand() == 3)
                {
                    offset = gep->get_operand(2);
                }
                if(array_map.find(lval) != array_map.end() && array_map[lval]->find(offset) != array_map[lval]->end())
                {
                    inst.replace_all_use_with(array_map[lval]->at(offset));
                    dead_inst.insert(&inst);
                }
            }
        }
        else if(inst.isBinary() || inst.is_cmp()|| inst.is_fcmp())
        {
         //   LOG_DEBUG << ("=====binary || cmp || fcmp=====");
            auto op1 = inst.get_operand(0);
            auto op2 = inst.get_operand(1);
            auto const_op1 = dynamic_cast<Constant *>(op1);
            auto const_op2 = dynamic_cast<Constant *>(op2);
            if(const_op1 && const_op2)
            {
                ConstFolder folder(m_);
                auto new_val = folder.compute(&inst, const_op1, const_op2);
                if(new_val)
                {
                    inst.replace_all_use_with(new_val);
               //     LOG_DEBUG << "const fold inst: " << inst.print();
                    dead_inst.insert(&inst);
                }
            }
        }
        else if(inst.is_fp2si() || inst.is_si2fp() || inst.is_zext())
        {
        //    LOG_DEBUG << ("=====fp2si || si2fp || zext=====");
            auto op = inst.get_operand(0);
            auto const_op = dynamic_cast<Constant *>(op);
            if(const_op)
            {
                ConstFolder folder(m_);
                auto new_val = folder.compute(&inst, const_op);
                if(new_val)
                {
          //          LOG_DEBUG << "const fold inst: " << inst.print();
                    inst.replace_all_use_with(new_val);
                    dead_inst.insert(&inst);
                }
            }
        }
        else if(inst.is_br())
        {
       //     LOG_DEBUG << ("=====br=====");
            auto br_inst = dynamic_cast<BranchInst *>(&inst);
            if(br_inst->is_cond_br())
            {
              //  LOG_DEBUG << ("=====cond br=====");
                auto truebb = dynamic_cast<BasicBlock*>(br_inst->get_operand(1));
                auto falsebb = dynamic_cast<BasicBlock*>(br_inst->get_operand(2));
                auto cond = br_inst->get_operand(0);
                auto const_cond = dynamic_cast<ConstantInt *>(cond);
                if(const_cond)
                {
                   // LOG_DEBUG << "const fold inst: " << inst.print();
                    falsebb->remove_pre_basic_block(bb);
                    truebb->remove_pre_basic_block(bb);
                    bb->remove_succ_basic_block(falsebb);
                    bb->remove_succ_basic_block(truebb);
                 //   LOG_DEBUG << const_cond->get_value() << " cond value";
                    if(const_cond->get_value() == 1)
                    {
                      //  dead_inst.insert(&inst);
                      //  LOG_DEBUG << "const fold to TRUEBB ";
                      //  bb->erase_instr(&inst);
                       // auto new_br = BranchInst::create_br(truebb, bb);
                        br_replace.insert(std::make_pair(&inst,truebb));
                       // bb->replace_instruction(&inst, new_br);
                      //  bb->add_instruction(new_br);
                        for(auto &inst : falsebb->get_instructions())
                        {
                            if(!inst.is_phi())
                            {
                                continue;
                            }
                            for(auto i = 1;i < inst.get_num_operand();i += 2)
                            {
                                if(inst.get_operand(i) == bb)
                                {
                                    inst.remove_operand(i);
                                    inst.remove_operand(i - 1);
                                }
                            }
                        }
                    }
                    else
                    {
                      //  dead_inst.insert(&inst);
                       // LOG_DEBUG << "const fold to FALSEBB ";
                       // bb->erase_instr(&inst);
                        //auto new_br = BranchInst::create_br(falsebb, bb);
                     //   LOG_DEBUG << "finish create";
                        //bb->replace_instruction(&inst, new_br);
                      //  bb->add_instruction(new_br);
                   //     LOG_DEBUG << "finish replace";
                        br_replace.insert(std::make_pair(&inst,falsebb));
                        for(auto &inst : truebb->get_instructions())
                        {
                            if(!inst.is_phi())
                            {
                                continue;
                            }
                            for(auto i = 1;i < inst.get_num_operand();i += 2)
                            {
                                if(inst.get_operand(i) == bb)
                                {
                                    inst.remove_operand(i);
                                    inst.remove_operand(i - 1);
                                }
                            }
                        }
                    }
                }
            }
        } 
    }
   // LOG_DEBUG << "br_replace size: " << br_replace.size();
    for(auto &br : br_replace)
    {
       // LOG_DEBUG << "br_replace inst: " << br.first->print();
        bb->erase_instr(br.first);
        auto new_br = BranchInst::create_br(br.second, bb);
       // bb->add_instruction(new_br);
    }
   // LOG_DEBUG << "dead_inst size: " << dead_inst.size();
   // for(auto &inst : dead_inst)
  //  {
   //     bb->erase_instr(inst);
   // }
}

